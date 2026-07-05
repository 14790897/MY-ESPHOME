# CLAUDE.md - AI Agent Context

## What is this repo?

ESPHome 智能家居设备配置集合。管理多个 ESP32/ESP32-C3/ESP32-S3 设备的固件配置，通过 Home Assistant 实现统一控制。

## Tech Stack

- **框架**: ESPHome 2026.6+
- **芯片**: ESP32-C3 (主力), ESP32-S3 (摄像头), ESP32 (通用)
- **开发板**: 合宙 CORE ESP32-C3 (`board: airm2m_core_esp32c3`)
- **平台**: PlatformIO + ESP-IDF / Arduino
- **智能家居**: Home Assistant
- **协议**: WiFi, ESP-NOW, BLE, UART

## Repo Structure

```
MY-ESPHOME/
├── configs/                     # 设备配置 (按功能分类)
│   ├── actuators/               # 执行器 (门锁, 舵机, LED, 指纹)
│   ├── environmental-sensors/   # 环境传感器 (VOC, 温湿度, 气压)
│   ├── motion-radar/            # 雷达人体检测 (LD2402/2410/2450)
│   ├── camera-ptz/              # 摄像头 + 云台
│   ├── displays/                # TFT 显示屏
│   ├── measurement-monitoring/  # 功率监控
│   ├── audio/                   # 音频设备
│   └── legacy/                  # 旧版配置
├── components/                  # 自定义 ESPHome External Components
├── common_components/           # 通用组件
├── common.yaml                  # 公共配置 (WiFi, API, OTA, Logger)
├── secrets.yaml                 # 密钥 (不提交 git)
├── ESPHOME_RULES.md             # 开发规则与踩坑记录
└── CLAUDE.md                    # 本文件
```

## Key Commands

> ⚠️ **运行 esphome 前必须先设置环境变量**（见 `.vscode/settings.json`）:
> ```powershell
> $env:PLATFORMIO_CORE_DIR = "C:\git-program\Embedded\MY-ESPHOME\.pio-core-esphome"
> $env:ESPHOME_CACHE_DIR = "C:\git-program\Embedded\MY-ESPHOME\.esphome"
> ```
> 不设置会使用全局 PlatformIO，导致框架版本冲突编译失败。

```bash
# 编译
esphome compile configs/actuators/door-espnow.yaml

# 编译 + USB 烧录
esphome run configs/actuators/door-gateway.yaml --device COM21

# OTA 无线更新 (仅限有 WiFi 的设备)
esphome run configs/xxx.yaml --device OTA

# 查看日志
esphome logs configs/xxx.yaml

# 读取芯片 MAC 地址
esptool.py --port COMx read_mac
```

## Conventions & Rules

### YAML 配置
- Entity `name` 只能用 **英文** (纯中文会 ASCII ID 冲突)
- `friendly_name` 可以中文
- 公共配置通过 `packages: base: !include ../../common.yaml` 引入
- 敏感信息放 `secrets.yaml`，用 `!secret key_name` 引用
- API key 必须是合法 base64 (32字节)，无 "base64:" 前缀

### 硬件
- 默认开发板: 合宙 CORE ESP32-C3，必须指定 `board: airm2m_core_esp32c3`
- Flash 模式: DIO (非 QIO)，GPIO12/13 可用
- GPIO9 是 strapping pin (BOOT键)，带 WARNING 但可用
- GPIO14-17 被 SPI Flash 占用，不可用
- GPIO11 默认给 Flash VDD，需烧 efuse 解锁

### ESP-IDF vs Arduino
- 需要 ESP-NOW / BLE / 高级 deep_sleep → 用 `esp-idf`
- 简单 WiFi 设备 → 用 `arduino` (编译更快)

### Deep Sleep
- lambda 中用 `id(x).prevent_deep_sleep()` / `allow_deep_sleep()`
- 不能用 `id(x).prevent()` (protected 成员)
- 无 WiFi 设备不支持 OTA，只能 USB 烧录

### ESP-NOW
- 需要 ESP-IDF 框架
- 无 WiFi 端需手动设 `channel` (匹配 AP 信道)
- 有 WiFi 端 channel 自动跟随，不可手动设
- 两端需互填对方 MAC 到 `peers` 列表
- 重试策略: 发送覆盖时间 > 接收端唤醒周期

## Active Devices

| 设备 | 配置文件 | 板型 | MAC |
|------|----------|------|-----|
| 门锁 (ESP-NOW) | `configs/actuators/door-espnow.yaml` | airm2m_core_esp32c3 | 34:CD:B0:A7:BC:00 |
| 门锁网关 | `configs/actuators/door-gateway.yaml` | airm2m_core_esp32c3 | 9C:13:9E:73:88:F4 |
| 门锁 (WiFi版) | `configs/actuators/door.yaml` | airm2m_core_esp32c3 | 34:CD:B0:A7:BC:00 |

## Important Files

- `ESPHOME_RULES.md` — 详细开发规则与踩坑汇总 (必读)
- `common.yaml` — 所有 WiFi 设备的公共配置基础
- `configs/actuators/secrets.yaml` — actuators 目录的密钥文件
