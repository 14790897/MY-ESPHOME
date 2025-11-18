# MY-ESPHOME

基于 ESPHome 的智能家居传感器和执行器配置集合。本项目包含多种 ESP32 设备的配置文件，涵盖环境监测、人体检测、摄像头控制、执行器等多个应用场景。

## 特性

- **多类型传感器支持**: VOC、温湿度、气压、雷达、惯性传感器等
- **执行器控制**: 舵机、LED、指纹识别模块
- **摄像头方案**: ESP32-S3 摄像头及云台控制
- **显示设备**: ILI9341 TFT 屏幕配置
- **自定义组件**: 符合 ESPHome 2025.2.0+ 标准的 External Component
- **Home Assistant 集成**: 无缝对接智能家居平台
- **OTA 更新**: 支持无线固件更新
- **统一配置**: 通用配置文件便于管理

## 目录结构

```
MY-ESPHOME/
├── configs/                    # 主要配置文件
│   ├── environmental-sensors/  # 环境传感器 (VOC, 温湿度, 气压等)
│   ├── motion-radar/           # 雷达传感器 (LD2402, LD2410, LD2450)
│   ├── camera-ptz/             # 摄像头与云台
│   ├── actuators/              # 执行器 (舵机, LED, 指纹)
│   ├── measurement-monitoring/ # 测量监控 (功率监控)
│   ├── displays/               # 显示设备
│   └── legacy/                 # 旧版配置
│
├── components/                 # 自定义组件
│   └── zw101/                  # ZW101 指纹识别模组
│
├── common_components/          # 通用组件
├── resources/                  # 资源文件 (字体, 图片)
├── test-code/                  # 测试代码
├── docs/                       # 技术文档
│
├── common.yaml                 # 通用配置
├── secrets.yaml                # 敏感信息配置
├── glyphs.yaml                 # 字形配置
├── README.md                   # 本文件
├── DIRECTORY_STRUCTURE.md      # 目录结构说明
└── ORGANIZATION.md             # 详细组织说明
```

## 环境准备

### 安装 ESPHome

```bash
# 使用 pip 安装
pip install esphome

# 或使用 Home Assistant 插件
# 在 Home Assistant 中安装 ESPHome 集成
```

### 配置 secrets.yaml

创建 `secrets.yaml` 文件存储敏感信息:

```yaml
wifi_ssid: "你的WiFi名称"
wifi_password: "你的WiFi密码"
```

## 快速开始

### 1. 编译配置文件

```bash
# 编译指定配置
esphome compile configs/environmental-sensors/21voc-esp32c3.yaml
```

### 2. 首次烧录 (通过USB)

```bash
# 运行 (编译 + 上传 + 日志)
esphome run configs/environmental-sensors/21voc-esp32c3.yaml

# 或指定串口
esphome run configs/environmental-sensors/21voc-esp32c3.yaml --device COM3
```

### 3. OTA 无线更新

```bash
# 通过 OTA 更新
esphome run configs/environmental-sensors/21voc-esp32c3.yaml --device OTA
```

### 4. 查看日志

```bash
esphome logs configs/environmental-sensors/21voc-esp32c3.yaml
```

## 主要设备配置

### 环境传感器

| 配置文件 | 设备 | 功能 |
|---------|------|------|
| `21voc-esp32c3.yaml` | 21合一VOC传感器 | VOC、甲醛、eCO2、温湿度 |
| `voc-esp32c3.yaml` | VOC-CO2-HCHO | 三合一空气质量检测 |
| `sht30-esp32c3.yaml` | SHT30 | 高精度温湿度 |
| `bmp_aht_esp32c3.yaml` | BMP280+AHT20 | 气压、温湿度 |

### 雷达传感器

| 配置文件 | 设备 | 功能 |
|---------|------|------|
| `ld2402-esp32c3.yaml` | LD2402 | 人体存在检测 |
| `ld2410-esp32c3.yaml` | LD2410 | 静态/运动检测 |
| `radar-esp32c3.yaml` | LD2450 | 多目标位置追踪 |

### 执行器与控制

| 配置文件 | 设备 | 功能 |
|---------|------|------|
| `servo-esp32c3.yaml` | 舵机 | PWM 舵机控制 |
| `led-esp32c3.yaml` | LED | 灯光控制 |
| `fingerprint-zw101-new.yaml` | ZW101指纹模组 | 指纹识别 + 门锁控制 |

### 摄像头

| 配置文件 | 设备 | 功能 |
|---------|------|------|
| `cam-esp32s3.yaml` | ESP32-S3 | 网络摄像头 |
| `PTZ-esp32c3.yaml` | PTZ云台 | 云台转动控制 |

## 自定义组件

### ZW101 指纹识别模组

本项目包含符合 ESPHome 2025.2.0+ 标准的 ZW101 指纹识别模组驱动。

**主要特性:**
- External Component 架构
- 自动搜索与注册
- LED 指示灯控制
- 与舵机联动实现智能门锁

**使用示例:**

```yaml
external_components:
  - source:
      type: local
      path: ../../components

zw101:
  id: zw101_reader
  uart_id: fingerprint_uart
```

详细文档: [components/zw101/README.md](components/zw101/README.md)

## 常用命令参考

```bash
# 编译固件
esphome compile <config_file.yaml>

# 上传固件 (首次通过USB)
esphome run <config_file.yaml> --device COM3

# OTA 更新
esphome run <config_file.yaml> --device OTA

# 查看日志
esphome logs <config_file.yaml>

# 清理缓存和 NVS
esphome run <config_file.yaml> --clean

# 清理构建文件
esphome clean <config_file.yaml>
```

## 配置文件说明

### common.yaml

包含所有设备共用的配置:
- WiFi 连接
- API 接口
- OTA 更新
- Logger
- Web Server

使用方式:
```yaml
<<: !include ../../common.yaml
```

### secrets.yaml

存储敏感信息，需要在每个配置目录下创建:
```yaml
wifi_ssid: "你的WiFi名称"
wifi_password: "你的WiFi密码"
```

## 注意事项

- **ESP32-C3 LED 引脚**: GPIO12 和 GPIO13 为内置 LED，输出高电平会导致闪烁
- **WiFi 配置**: AP 模式设置的 WiFi 会保存在 NVS，使用 `--clean` 清除
- **电源要求**: 某些传感器（如 ZW101）需要独立 5V 电源供电
- **串口波特率**: 不同设备使用不同波特率，请参考各配置文件



## 文档资源

- [目录结构说明](DIRECTORY_STRUCTURE.md) - 快速了解项目结构
- [组织说明](ORGANIZATION.md) - 详细的设备分类和管理
- [故障排除指南](docs/TROUBLESHOOTING.md) - 常见问题解决
- [ZW101 指纹模组](components/zw101/README.md) - 指纹识别详细文档

## 参考链接

- [ESPHome 官方文档](https://esphome.io/)
- [ESPHome External Components](https://esphome.io/components/external_components/)
- [Home Assistant](https://www.home-assistant.io/)
- [ESP32 开发文档](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/)

## 贡献

欢迎提交 Issue 和 Pull Request!

## 许可

MIT License