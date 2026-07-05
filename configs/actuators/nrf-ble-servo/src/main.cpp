/**
 * BLE 舵机控制器 for nRF52840
 * 
 * 移植自 ESP-NOW 门锁 (main.c) 的全部功能:
 *   - 舵机控制: OPEN / CLOSE / ANGLE:N (0-180°)
 *   - 工作模式: DAY (短广播间隔) / NIGHT (长广播间隔, 省电)
 *   - LED 状态指示
 *   - 命令 ACK 回执
 *   - 启动日志/状态上报
 * 
 * 指令 (字符串方式):
 *   "OPEN"      - 开门 (0°)
 *   "CLOSE"     - 关门 (180°)  
 *   "ANGLE:90"  - 指定角度
 *   "NIGHT"     - 夜间模式 (省电, 长广播间隔)
 *   "DAY"       - 白天模式 (正常, 短广播间隔)
 *   "STATUS"    - 查询当前状态
 * 
 * 指令 (十六进制方式, 与 ESP-NOW 门锁协议对齐):
 *   0x01 = OPEN
 *   0x02 = CLOSE
 *   0x03 = NIGHT
 *   0x04 = DAY
 *   "ANGLE:0xNN" = 指定角度 (0x00-0xB4)
 * 
 * 硬件: Adafruit Feather nRF52840 + MG946R 舵机
 * 省电: NIGHT 模式广播间隔 10s → 功耗极低
 */

#include <Arduino.h>
#include <bluefruit.h>
#include <Servo.h>

// ===================================================================
// 硬件引脚配置
// ===================================================================
#define SERVO_PIN       5       // 舵机 PWM 引脚
#define LED_STATUS_PIN  LED_BUILTIN  // 状态 LED (板载)
// 第二 LED (可选, 如无则注释掉或与 LED_BUILTIN 同引脚)
#ifndef PIN_LED2
#define LED2_PIN        16      // 第二 LED (Adafruit nRF52840: 16 = 蓝色)
#endif

// ===================================================================
// 舵机参数
// ===================================================================
#define SERVO_OPEN_DEG   0       // 开门角度
#define SERVO_CLOSE_DEG  180     // 关门角度
#define SERVO_HOLD_MS    2000    // 舵机保持通电时间

// ===================================================================
// BLE UUID (128-bit, 与原始保持一致)
// ===================================================================
BLEUuid svc_uuid("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEUuid tx_uuid("19B10001-E8F2-537E-4F6C-D104768A1214");  // Notify (TX)
BLEUuid rx_uuid("19B10002-E8F2-537E-4F6C-D104768A1214");  // Write  (RX)

BLECharacteristic tx, rx;

// ===================================================================
// 工作模式
// ===================================================================
// DAY:  正常广播间隔 (快速响应)
// NIGHT: 省电长广播间隔 (~10s, 极低功耗, 首次连接可能稍慢)
#define ADV_INTERVAL_DAY_MS    100     // 白天广播间隔
#define ADV_INTERVAL_NIGHT_MS  10000   // 夜间广播间隔 (10s, 省电)

// ===================================================================
// 全局状态
// ===================================================================
Servo servo;
static bool night_mode = false;   // 夜间模式标志
static bool servo_active = false; // 舵机是否正在动作

// ===================================================================
// LED 控制
// ===================================================================
static void led_init(void)
{
    pinMode(LED_STATUS_PIN, OUTPUT);
    digitalWrite(LED_STATUS_PIN, LOW);
#ifdef LED2_PIN
    pinMode(LED2_PIN, OUTPUT);
    digitalWrite(LED2_PIN, LOW);
#endif
}

// 快闪 N 次 (用于指示命令执行)
static void led_flash(int count, int delay_ms)
{
    for (int i = 0; i < count; i++) {
        digitalWrite(LED_STATUS_PIN, HIGH);
        delay(delay_ms);
        digitalWrite(LED_STATUS_PIN, LOW);
        if (i < count - 1) delay(delay_ms);
    }
}

// 设置状态 LED (常亮/灭)
static void led_set(bool on)
{
    digitalWrite(LED_STATUS_PIN, on ? HIGH : LOW);
}

// ===================================================================
// 舵机控制
// ===================================================================
static void servo_do_open(void)
{
    servo_active = true;
    led_flash(1, 100);
    servo.attach(SERVO_PIN);
    servo.write(SERVO_OPEN_DEG);
    delay(SERVO_HOLD_MS);
    servo.detach();
    servo_active = false;
    led_set(LOW);
}

static void servo_do_close(void)
{
    servo_active = true;
    led_flash(2, 100);
    servo.attach(SERVO_PIN);
    servo.write(SERVO_CLOSE_DEG);
    delay(SERVO_HOLD_MS);
    servo.detach();
    servo_active = false;
    led_set(LOW);
}

static void servo_do_angle(int angle)
{
    if (angle < 0 || angle > 180) return;
    servo_active = true;
    led_flash(1, 50);
    servo.attach(SERVO_PIN);
    servo.write(angle);
    delay(SERVO_HOLD_MS);
    servo.detach();
    servo_active = false;
    led_set(LOW);
}

// ===================================================================
// 模式切换
// ===================================================================
static void set_day_mode(void)
{
    night_mode = false;
    Bluefruit.Advertising.stop();
    Bluefruit.Advertising.setInterval(ADV_INTERVAL_DAY_MS, ADV_INTERVAL_DAY_MS);
    Bluefruit.Advertising.start(0);  // 0 = 无限广播
    led_flash(3, 80);
    Serial.println("Mode: DAY (fast advertising)");
}

static void set_night_mode(void)
{
    night_mode = true;
    Bluefruit.Advertising.stop();
    Bluefruit.Advertising.setInterval(ADV_INTERVAL_NIGHT_MS, ADV_INTERVAL_NIGHT_MS);
    Bluefruit.Advertising.start(0);  // 0 = 无限广播
    led_flash(1, 300);
    Serial.println("Mode: NIGHT (low power advertising)");
}

// ===================================================================
// 状态上报
// ===================================================================
static void report_status(void)
{
    char buf[64];
    snprintf(buf, sizeof(buf), 
        "STATUS:NIGHT=%d,SERVO=%d,ADV=%lums,UPTIME=%lu",
        night_mode ? 1 : 0,
        servo_active ? 1 : 0,
        night_mode ? (unsigned long)ADV_INTERVAL_NIGHT_MS : (unsigned long)ADV_INTERVAL_DAY_MS,
        (unsigned long)(millis() / 1000));
    tx.notify(buf, strlen(buf));
    Serial.println(buf);
}

// ===================================================================
// BLE 接收回调
// ===================================================================
void rx_cb(uint16_t conn_handle, BLECharacteristic *chr, uint8_t *data, uint16_t len)
{
    if (len == 0) return;

    // ─── 判断是字符串命令还是十六进制命令 ───
    // 如果第一个字节是 0x00-0x04 且长度=1, 按十六进制协议处理
    if (len == 1 && data[0] <= 0x04) {
        switch (data[0]) {
            case 0x01:  // CMD_OPEN
                Serial.println("CMD: OPEN (hex)");
                tx.notify("OK:OPEN");
                servo_do_open();
                break;
            case 0x02:  // CMD_CLOSE
                Serial.println("CMD: CLOSE (hex)");
                tx.notify("OK:CLOSE");
                servo_do_close();
                break;
            case 0x03:  // CMD_NIGHT
                Serial.println("CMD: NIGHT (hex)");
                tx.notify("OK:NIGHT");
                set_night_mode();
                break;
            case 0x04:  // CMD_DAY
                Serial.println("CMD: DAY (hex)");
                tx.notify("OK:DAY");
                set_day_mode();
                break;
            default:
                tx.notify("ERR:UNKNOWN");
                break;
        }
        return;
    }

    // ─── 字符串命令解析 ───
    String cmd;
    cmd.reserve(len);
    for (uint16_t i = 0; i < len; i++) {
        cmd += (char)data[i];
    }
    Serial.print("CMD(string): ");
    Serial.println(cmd);

    if (cmd == "OPEN") {
        tx.notify("OK:OPEN");
        servo_do_open();
    }
    else if (cmd == "CLOSE") {
        tx.notify("OK:CLOSE");
        servo_do_close();
    }
    else if (cmd == "NIGHT") {
        tx.notify("OK:NIGHT");
        set_night_mode();
    }
    else if (cmd == "DAY") {
        tx.notify("OK:DAY");
        set_day_mode();
    }
    else if (cmd == "STATUS") {
        report_status();
    }
    else if (cmd.startsWith("ANGLE:")) {
        String angle_str = cmd.substring(6);
        // 支持十六进制: "ANGLE:0x5A"
        int angle;
        if (angle_str.startsWith("0x") || angle_str.startsWith("0X")) {
            angle = (int)strtol(angle_str.c_str() + 2, NULL, 16);
        } else {
            angle = angle_str.toInt();
        }
        if (angle >= 0 && angle <= 180) {
            char buf[16];
            snprintf(buf, sizeof(buf), "OK:%d", angle);
            tx.notify(buf, strlen(buf));
            servo_do_angle(angle);
        } else {
            tx.notify("ERR:BAD_ANGLE");
        }
    }
    else {
        tx.notify("ERR:UNKNOWN");
    }
}

// ===================================================================
// BLE 连接/断开回调
// ===================================================================
void connect_cb(uint16_t conn_handle)
{
    Serial.println("BLE Connected");
    led_set(HIGH);
    // 连接后上报一次状态
    delay(100);
    report_status();
}

void disconnect_cb(uint16_t conn_handle, uint8_t reason)
{
    Serial.printf("BLE Disconnected, reason: %d\n", reason);
    led_set(LOW);
}

// ===================================================================
// 初始化
// ===================================================================
void setup()
{
    Serial.begin(115200);
    
    // 记录启动时间
    uint32_t boot_ms = millis();
    
    Serial.println("========================================");
    Serial.println("  BLE Servo Controller for nRF52840");
    Serial.println("  Ported from ESP-NOW Door Lock");
    Serial.println("========================================");

    // LED 初始化
    led_init();
    led_flash(1, 200);  // 启动指示

    // 舵机初始化
    servo.attach(SERVO_PIN);
    servo.write(SERVO_OPEN_DEG);  // 初始位置: 开门
    delay(500);
    servo.detach();

    // BLE 初始化
    Bluefruit.begin();
    Bluefruit.setName("NRF-Servo");
    Bluefruit.autoConnLed(false);       // 彻底禁用连接状态 LED (蓝灯)
    Bluefruit.setConnLedInterval(0);    // 连接 LED 闪烁间隔 = 0
    Bluefruit.Periph.setConnectCallback(connect_cb);
    Bluefruit.Periph.setDisconnectCallback(disconnect_cb);

    // 创建服务
    BLEService s(svc_uuid);
    s.begin();

    // TX Characteristic (Notify - 发送数据给手机/网关)
    tx.setProperties(CHR_PROPS_NOTIFY);
    tx.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    tx.setUuid(tx_uuid);
    tx.begin();

    // RX Characteristic (Write - 接收命令)
    rx.setProperties(CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
    rx.setPermission(SECMODE_OPEN, SECMODE_OPEN);
    rx.setUuid(rx_uuid);
    rx.setWriteCallback(rx_cb);
    rx.begin();

    // 广播设置
    Bluefruit.Advertising.addService(s);
    Bluefruit.Advertising.addName();
    Bluefruit.Advertising.setInterval(ADV_INTERVAL_DAY_MS, ADV_INTERVAL_DAY_MS);
    Bluefruit.Advertising.setFastTimeout(30);  // 30s 快速广播后自动切换
    Bluefruit.Advertising.start();

    uint32_t init_ms = millis() - boot_ms;
    Serial.printf("Init done in %lu ms, advertising...\n", init_ms);
    Serial.printf("Day advertising interval: %d ms\n", ADV_INTERVAL_DAY_MS);
    Serial.printf("Night advertising interval: %d ms\n", ADV_INTERVAL_NIGHT_MS);
}

// ===================================================================
// 主循环 (空闲)
// ===================================================================
void loop()
{
    // Bluefruit 内部处理在中断中完成
    // 此处可添加额外逻辑 (如周期性状态日志)
    
    // 每 60 秒串口输出一次状态 (调试用)
    static uint32_t last_report = 0;
    if (millis() - last_report > 60000) {
        last_report = millis();
        Serial.printf("[HEARTBEAT] night_mode=%d, servo_active=%d, uptime=%lus\n",
                      night_mode, servo_active, (unsigned long)(millis() / 1000));
    }
    
    // nRF52 在 loop() 空闲时自动进入低功耗 (__WFE)
    yield();
}
