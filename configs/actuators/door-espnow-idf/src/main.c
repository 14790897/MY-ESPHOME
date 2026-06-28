/**
 * ESP-NOW 门锁接收端 - 裸 ESP-IDF 版本
 * 
 * 超低功耗方案：周期性唤醒监听 ESP-NOW 命令，驱动舵机开/关门
 * 
 * 硬件: 合宙 CORE ESP32-C3 + MG946R 舵机
 * 功耗: ~2.4mA 平均 (30ms listen / 1s sleep)
 * 续航: 10000mAh → ~170 天
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_idf_version.h"

// ===================================================================
// 配置参数
// ===================================================================

// ESP-NOW
#define ESPNOW_CHANNEL      1       // WiFi 信道 (必须与网关 WiFi AP 一致)
#define LISTEN_DURATION_MS  30      // 监听窗口 (ms)
#define SLEEP_DURATION_US   1000000 // 深睡时长 (us) = 1s

// 网关 MAC 地址 (修改为你的网关 MAC)
static const uint8_t GATEWAY_MAC[6] = {0x9C, 0x13, 0x9E, 0x73, 0x88, 0xF4};

// 命令协议
#define CMD_OPEN        0x01
#define CMD_CLOSE       0x02
#define CMD_NIGHT       0x03    // 夜间模式 (长休眠)
#define CMD_DAY         0x04    // 白天模式 (1s 唤醒)
#define CMD_ACK_OPEN    0x10
#define CMD_ACK_CLOSE   0x20
#define CMD_ACK_SLEEP   0x30    // 模式切换回执

// 夜间休眠时长 (秒) = 一次唤醒检查的间隔
#define NIGHT_SLEEP_SEC 20

// 舵机 PWM (GPIO9, 100Hz)
#define SERVO_GPIO          GPIO_NUM_9
#define SERVO_FREQ_HZ       100
#define SERVO_DUTY_OPEN     819     // 12.5% of 8191 (13-bit) ≈ 1.25ms
#define SERVO_DUTY_CLOSE    2048    // 25% of 8191 ≈ 2.5ms
#define SERVO_HOLD_MS       2000    // 舵机保持通电时间

// LED (GPIO12/13, 高电平有效, 关闭省电)
#define LED_D4_GPIO         GPIO_NUM_12
#define LED_D5_GPIO         GPIO_NUM_13

// ===================================================================
// 全局状态
// ===================================================================
//
//  数据流向:
//    espnow_recv_cb() ──写入──▶ received_cmd / command_received ──读取──▶ app_main()
//    espnow_recv_cb() ──写入──▶ night_mode                      ──读取──▶ enter_deep_sleep()
//
//  为什么用全局变量？
//    esp_now_register_recv_cb() 不支持 user_data 参数，回调签名固定，
//    只能通过 volatile 全局变量与主循环通信。
// ===================================================================

static const char *TAG = "door";

// [写入: espnow_recv_cb()] [读取: app_main() switch()]
static volatile bool command_received = false;
static volatile uint8_t received_cmd = 0;

// [写入: espnow_recv_cb()] [读取: enter_deep_sleep()]  deep sleep 期间保持
RTC_DATA_ATTR static bool night_mode = false;

// ===================================================================
// ESP-NOW 回调
// ===================================================================

// ESP-NOW 回调 — 兼容 ESP-IDF v4.x 和 v5.x
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    const uint8_t *src = info->src_addr;
#else
static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    const uint8_t *src = mac_addr;
#endif
    if (len < 1 || command_received) return;

    if (memcmp(src, GATEWAY_MAC, 6) != 0) {
        ESP_LOGW(TAG, "Unknown sender, ignoring");
        return;
    }

    received_cmd = data[0];
    command_received = true;
    ESP_LOGI(TAG, "Command received: 0x%02X", received_cmd);

    if (received_cmd == CMD_NIGHT) {
        night_mode = true;
        ESP_LOGI(TAG, "Entering night mode");
    } else if (received_cmd == CMD_DAY) {
        night_mode = false;
        ESP_LOGI(TAG, "Entering day mode");
    }
}

// ===================================================================
// 舵机控制
// ===================================================================

static void servo_init(void)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,  // 8192 levels
        .timer_num = LEDC_TIMER_0,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf = {
        .gpio_num = SERVO_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&channel_conf);
}

static void servo_write(uint32_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void servo_detach(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
}

// ===================================================================
// WiFi + ESP-NOW 初始化
// ===================================================================

static void wifi_init(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    // 关闭省电，确保快速收发
    esp_wifi_set_ps(WIFI_PS_NONE);
}

static void espnow_init(void)
{
    esp_now_init();
    esp_now_register_recv_cb(espnow_recv_cb);

    // 添加网关为 peer
    esp_now_peer_info_t peer = {
        .channel = ESPNOW_CHANNEL,
        .encrypt = false,
    };
    memcpy(peer.peer_addr, GATEWAY_MAC, 6);
    esp_now_add_peer(&peer);
}

static void send_ack(uint8_t ack)
{
    esp_now_send(GATEWAY_MAC, &ack, 1);
}

// ===================================================================
// LED 关闭 (省电)
// ===================================================================

static void leds_off(void)
{
    // 释放上次 deep sleep 的 hold (否则无法改变 GPIO 状态)
    gpio_hold_dis(LED_D4_GPIO);
    gpio_hold_dis(LED_D5_GPIO);

    // 拉低 GPIO → LED 灭
    gpio_set_direction(LED_D4_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_D5_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_D4_GPIO, 0);
    gpio_set_level(LED_D5_GPIO, 0);

    // 锁住 GPIO 状态，deep sleep 期间保持
    // ESP32-C3 所有 GPIO 均支持 deep sleep hold (不限 RTC GPIO)
    gpio_hold_en(LED_D4_GPIO);
    gpio_hold_en(LED_D5_GPIO);
    gpio_deep_sleep_hold_en();
}

// ===================================================================
// 深睡
// ===================================================================

static void enter_deep_sleep(void)
{
    // 清理
    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();

    uint64_t sleep_us;
    if (night_mode) {
        sleep_us = NIGHT_SLEEP_SEC * 1000000ULL;
        ESP_LOGI(TAG, "Night mode: sleeping %d s (until morning)", NIGHT_SLEEP_SEC);
    } else {
        sleep_us = SLEEP_DURATION_US;
    }

    esp_sleep_enable_timer_wakeup(sleep_us);
    esp_deep_sleep_start();
}

// ===================================================================
// 主程序
// ===================================================================

void app_main(void)
{
    // 记录启动时间
    int64_t boot_time = esp_timer_get_time();

    ESP_LOGI(TAG, "=== Door Lock ESP-NOW (IDF) ===");
    ESP_LOGI(TAG, "Boot reason: %d, night_mode: %d", esp_sleep_get_wakeup_cause(), night_mode);

    // 关闭 LED
    leds_off();

    // NVS 初始化 (WiFi 需要)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 初始化 WiFi + ESP-NOW
    wifi_init();
    espnow_init();

    int64_t init_time = esp_timer_get_time();
    ESP_LOGI(TAG, "Init took %lld us, listening for %d ms...",
             init_time - boot_time, LISTEN_DURATION_MS);

    // 监听窗口
    vTaskDelay(pdMS_TO_TICKS(LISTEN_DURATION_MS));

    if (command_received) {
        switch (received_cmd) {
            case CMD_OPEN:
                ESP_LOGI(TAG, "OPEN - servo to 0 deg");
                servo_init();
                send_ack(CMD_ACK_OPEN);
                servo_write(SERVO_DUTY_OPEN);
                vTaskDelay(pdMS_TO_TICKS(SERVO_HOLD_MS));
                servo_detach();
                break;

            case CMD_CLOSE:
                ESP_LOGI(TAG, "CLOSE - servo to 180 deg");
                servo_init();
                send_ack(CMD_ACK_CLOSE);
                servo_write(SERVO_DUTY_CLOSE);
                vTaskDelay(pdMS_TO_TICKS(SERVO_HOLD_MS));
                servo_detach();
                break;

            case CMD_NIGHT:
            case CMD_DAY:
                send_ack(CMD_ACK_SLEEP);
                // 模式已在回调中切换，不做其他操作
                break;

            default:
                break;
        }
    } else {
        ESP_LOGD(TAG, "No command, going back to sleep");
    }

    // 进入深睡
    enter_deep_sleep();
}
