# 门锁 ESP-NOW 接收端 (ESP-IDF 版本)

裸 ESP-IDF 实现，相比 ESPHome 版本启动时间从 ~200ms 降到 ~20ms，功耗大幅降低。

## 功耗对比

| 方案 | 启动+监听时间 | 平均电流 | 10000mAh 续航 |
|------|-------------|---------|--------------|
| ESPHome (door-espnow.yaml) | ~200ms | ~13mA | ~31 天 |
| **ESP-IDF (本项目)** | **~30ms** | **~2.4mA** | **~170 天** |

## 配置修改

编辑 `src/main.c` 顶部的配置参数：

```c
#define ESPNOW_CHANNEL      1       // 改为你的 WiFi AP 信道
#define LISTEN_DURATION_MS  30      // 监听窗口 (ms)
#define SLEEP_DURATION_US   1000000 // 1s 深睡

// 网关 MAC (改为你的网关)
static const uint8_t GATEWAY_MAC[6] = {0x9C, 0x13, 0x9E, 0x73, 0x88, 0xF4};
```

## 编译与烧录 (PlatformIO)

> ⚠️ **重要：** 本项目位于 ESPHome 仓库内，ESPHome 自带的 `.pio-core-esphome/` 会干扰 PlatformIO 环境选择。必须在运行 `pio` 前设置环境变量指向独立 PlatformIO。

**PowerShell:**

```powershell
# 编译
$env:PLATFORMIO_CORE_DIR = "$env:USERPROFILE\.platformio"; pio run

# 编译 + 烧录
$env:PLATFORMIO_CORE_DIR = "$env:USERPROFILE\.platformio"; pio run -t upload --upload-port COM18

# 查看日志
$env:PLATFORMIO_CORE_DIR = "$env:USERPROFILE\.platformio"; pio device monitor --port COM18 --baud 115200
```

**Bash (Git Bash / WSL):**

```bash
PLATFORMIO_CORE_DIR=$HOME/.platformio pio run
PLATFORMIO_CORE_DIR=$HOME/.platformio pio run -t upload --upload-port COM18
```

### 常用命令

```bash
# 编译
pio run

# 编译 + 烧录 (指定端口)
pio run -t upload

# 仅烧录 (已编译过)
pio run -t upload

# 查看串口日志
pio device monitor --baud 115200

# 清理编译缓存
pio run -t clean

# 完全清理 (含依赖)
pio run -t fullclean

# 查看板子信息
pio boards airm2m_core_esp32c3

# 清理 PlatformIO 系统缓存 (节省磁盘)
pio system prune

# 列出可用串口
pio device list
```

### 项目结构

```
door-espnow-idf/
├── platformio.ini       # PlatformIO 配置 (板子、框架、flash 模式)
├── src/
│   └── main.c           # 固件源码
└── README.md
```

### 依赖

| 工具 | 版本 |
|------|------|
| PlatformIO | 通过系统 Python 安装 |
| ESP-IDF | v4.4.5 (PlatformIO 自动下载) |
| 板子 | `airm2m_core_esp32c3` (合宙 CORE ESP32-C3) |

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
- **白天 1s 唤醒是为了正常响应开门命令，夜间 60s 唤醒是为了防止充电宝休眠断电**
