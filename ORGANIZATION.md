# 🗂️ ESPHome配置文件组织结构

本目录已重新组织，将所有ESPHome配置文件按功能分类，便于管理和维护。

## 📁 目录结构

### `/configs/` - 主要配置文件
ESPHome设备配置文件按功能分类存放

#### 🌡️ `/configs/environmental-sensors/` - 环境传感器
环境监测相关的传感器配置

| 配置文件 | 设备描述 | 主要功能 |
|---------|----------|----------|
| `21voc-esp32c3.yaml` | 21合一VOC传感器 | VOC、甲醛、eCO2、温湿度检测 |
| `voc-esp32c3.yaml` | VOC-CO2-HCHO传感器 | 三合一空气质量检测 |
| `ENS160.yaml` | ENS160传感器 | 空气质量传感器 |
| `bmi160-esp32c3.yaml` | BMI160传感器 | 6轴惯性传感器(加速度+陀螺仪) |
| `bmp_aht_esp32c3.yaml` | BMP280+AHT20传感器 | 气压、温湿度检测 |
| `bmp_aht_esp32c3-ld2450.yaml` | BMP280+AHT20+LD2450 | 环境传感器+雷达复合配置 |
| `sht30-esp32c3.yaml` | SHT30传感器 | 高精度温湿度传感器 |
| `sht30-sensor.yaml` | SHT30传感器(简化版) | 基础温湿度功能 |

#### 📡 `/configs/motion-radar/` - 雷达传感器
人体检测和运动感知雷达配置

| 配置文件 | 设备描述 | 主要功能 |
|---------|----------|----------|
| `ld2402-esp32c3.yaml` | LD2402雷达传感器 | 人体存在检测 |
| `ld2410-esp32c3.yaml` | LD2410雷达传感器 | 静态/运动人体检测 |
| `radar-esp32c3.yaml` | LD2450雷达传感器 | 多目标位置追踪 |

#### 📷 `/configs/camera-ptz/` - 摄像头与云台
摄像头和云台控制配置

| 配置文件 | 设备描述 | 主要功能 |
|---------|----------|----------|
| `cam-esp32s3.yaml` | ESP32-S3摄像头 | 网络摄像头功能 |
| `PTZ-esp32c3.yaml` | PTZ云台控制 | 云台转动控制 |
| `esp32s3-n8.yaml` | ESP32-S3-N8 | 高性能摄像头模块 |

#### ⚙️ `/configs/actuators/` - 执行器
各种执行器和控制设备配置

| 配置文件 | 设备描述 | 主要功能 |
|---------|----------|----------|
| `servo-esp32c3.yaml` | 舵机控制 | PWM舵机驱动 |
| `led-esp32c3.yaml` | LED控制 | 灯光控制系统 |

#### 📊 `/configs/measurement-monitoring/` - 测量监控
电力和其他参数监控配置

| 配置文件 | 设备描述 | 主要功能 |
|---------|----------|----------|
| `ina226-esp32c3.yaml` | INA226功率监控 | 电压、电流、功率测量 |

#### 🖥️ `/configs/displays/` - 显示设备
各种显示屏配置

| 配置文件 | 设备描述 | 主要功能 |
|---------|----------|----------|
| `display9341.yaml` | ILI9341显示屏 | TFT彩色显示屏 |
| `glyphs.yaml` | 字形配置 | 显示屏字体和图标 |
| `esp32s3-n8.yaml` | ESP32-S3显示面板(WiFi版) | 从HA获取传感器数据显示 |
| `envpanel-espnow.yaml` | **ESP32-S3显示面板(ESP-NOW版)** ⭐ | 通过ESP-NOW接收传感器数据 |

#### 🗄️ `/configs/legacy/` - 旧版配置
不再活跃维护的旧版本配置

| 配置文件 | 设备描述 | 备注 |
|---------|----------|------|
| `my8266.yaml` | ESP8266设备 | 旧版本，建议升级到ESP32 |

### 📁 `/resources/` - 资源文件
各种资源和素材文件

#### 🔤 `/resources/fonts/` - 字体文件
| 文件 | 描述 |
|------|------|
| `NotoSansSC-*.ttf` | 思源黑体中文字体系列 |
| `OFL.txt` | 开源字体许可证 |

#### 🖼️ `/resources/images/` - 图片资源
| 文件 | 描述 |
|------|------|
| `8266_datasheet.png` | ESP8266数据表 |
| `esp32s3cam.png` | ESP32-S3摄像头图片 |

#### 📦 `/resources/static/` - 静态资源
Web界面和其他静态文件

### 🧪 `/test-code/` - 测试代码
原始Arduino测试代码和参考实现

| 文件 | 描述 |
|------|------|
| `test_VOC-CO2-HCHO-Sensor.cpp` | VOC传感器测试程序 |

### 📋 `/docs/` - 文档
技术文档和说明文件

#### 📖 可用文档
- `TROUBLESHOOTING.md` - **故障排除指南** ⭐
  - PlatformIO 编译错误解决方案
  - 文件包含路径问题修复
  - ESP32 框架兼容性问题
  - GPIO 引脚配置指南
  - 环境变量配置方法
- `PLATFORMIO_VERSION_MANAGEMENT.md` - PlatformIO 平台版本管理指南

- `sht30-uart/` - SHT30 UART传感器技术文档

## 🚀 使用方法

### 基本命令
```bash
# 编译配置
esphome compile configs/environmental-sensors/21voc-esp32c3.yaml

# 上传固件
esphome run configs/environmental-sensors/21voc-esp32c3.yaml

# OTA更新
esphome run configs/environmental-sensors/21voc-esp32c3.yaml --device OTA
```

### 推荐工作流程
1. **开发阶段**: 在相应分类目录中编辑配置文件
2. **测试阶段**: 使用`esphome compile`验证配置
3. **部署阶段**: 使用`esphome run`上传到设备
4. **维护阶段**: 使用OTA进行无线更新

## 🔧 配置管理

### 新增设备配置
1. 确定设备类型，选择对应的分类目录
2. 在该目录下创建新的YAML配置文件
3. 更新本文档的设备列表

### 迁移现有配置
如需迁移配置到其他目录：
```bash
# 移动文件到新分类
Move-Item "old-config.yaml" "configs/new-category/"

# 更新相关文档和脚本中的路径引用
```

## 🌟 配置特色

### 高质量传感器支持
- ✅ 21合一VOC传感器(完整实现)
- ✅ 多种雷达传感器(LD2402/LD2410/LD2450)
- ✅ 环境传感器组合配置
- ✅ 摄像头和PTZ控制

### 统一设计理念
- 🔧 一致的YAML结构
- 📡 统一的MQTT配置
- 🏠 Home Assistant自动发现
- 🌐 Web管理界面
- 🔄 OTA无线更新

### 企业级功能
- 📊 详细的日志记录
- ⚡ 优化的性能配置
- 🛡️ 错误处理和恢复
- 📈 数据质量验证

## 📞 维护说明

- 定期更新ESPHome版本
- 检查配置文件的兼容性
- 备份重要的配置文件
- 记录设备部署位置和用途

---
**📅 最后更新**: 2025年9月12日  
**👤 维护者**: ESP32C3传感器框架团队  
**📧 反馈**: 如有问题请查看README.md或相关技术文档
