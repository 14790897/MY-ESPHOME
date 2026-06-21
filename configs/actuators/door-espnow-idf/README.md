# 门锁 ESP-NOW 接收端 (ESP-IDF 版本)

裸 ESP-IDF 实现，相比 ESPHome 版本启动时间从 ~200ms 降到 ~20ms，功耗大幅降低。

## 功耗对比

| 方案 | 启动+监听时间 | 平均电流 | 10000mAh 续航 |
|------|-------------|---------|--------------|
| ESPHome (door-espnow.yaml) | ~200ms | ~13mA | ~31 天 |
| **ESP-IDF (本项目)** | **~30ms** | **~2.4mA** | **~170 天** |

## 配置修改

编辑 `main/main.c` 顶部的配置参数：

```c
#define ESPNOW_CHANNEL      1       // 改为你的 WiFi AP 信道
#define LISTEN_DURATION_MS  30      // 监听窗口 (ms)
#define SLEEP_DURATION_US   1000000 // 1s 深睡

// 网关 MAC (改为你的网关)
static const uint8_t GATEWAY_MAC[6] = {0x9C, 0x13, 0x9E, 0x73, 0x88, 0xF4};
```

## 编译与烧录

### 前提

- 安装 ESP-IDF v5.x
- 设置 target 为 ESP32-C3

### 命令

```bash
# 设置 target
idf.py set-target esp32c3

# 编译
idf.py build

# 烧录 (指定串口)
idf.py -p COM20 flash

# 查看日志
idf.py -p COM20 monitor
```

### 使用 PlatformIO

```ini
; platformio.ini
[env:door-espnow-idf]
platform = espressif32
board = airm2m_core_esp32c3
framework = espidf
monitor_speed = 115200
```

## 与 ESPHome 网关协作

本固件与 `door-gateway.yaml`（ESPHome 网关）配合使用，协议完全兼容：

- 0x01 = 开门
- 0x02 = 关门
- 0x10 = ACK 开门
- 0x20 = ACK 关门

网关无需修改，直接使用现有的 `door-gateway.yaml` 即可。

## 注意事项

- GPIO12/13 LED 在 deep sleep 期间会亮（非 RTC GPIO），建议硬件拆除
- GPIO9 (BOOT) 用于舵机 PWM，是 strapping pin，外接舵机时不要下拉
- 无 OTA 能力，每次更新需 USB 烧录
