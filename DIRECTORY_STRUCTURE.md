# 📂 ESPHome配置文件目录组织

## 🗂️ 新目录结构

```
MY-ESPHOME/
├── configs/                      # 主要配置文件
│   ├── environmental-sensors/    # 🌡️ 环境传感器
│   │   ├── 21voc-esp32c3.yaml   # 21合一VOC传感器
│   │   ├── voc-esp32c3.yaml     # VOC-CO2-HCHO传感器  
│   │   ├── ENS160.yaml          # ENS160空气质量
│   │   ├── bmi160-esp32c3.yaml  # BMI160惯性传感器
│   │   ├── bmp_aht_esp32c3.yaml # 气压温湿度传感器
│   │   └── sht30-esp32c3.yaml   # SHT30温湿度传感器
│   │
│   ├── motion-radar/             # 📡 雷达传感器
│   │   ├── ld2402-esp32c3.yaml  # LD2402人体检测
│   │   ├── ld2410-esp32c3.yaml  # LD2410运动检测
│   │   └── radar-esp32c3.yaml   # LD2450多目标追踪
│   │
│   ├── camera-ptz/               # 📷 摄像头云台
│   │   ├── cam-esp32s3.yaml     # ESP32-S3摄像头
│   │   ├── PTZ-esp32c3.yaml     # PTZ云台控制
│   │   └── esp32s3-n8.yaml      # 高性能摄像头
│   │
│   ├── actuators/                # ⚙️ 执行器
│   │   ├── servo-esp32c3.yaml   # 舵机控制
│   │   └── led-esp32c3.yaml     # LED控制
│   │
│   ├── measurement-monitoring/   # 📊 测量监控  
│   │   └── ina226-esp32c3.yaml  # 功率监控
│   │
│   ├── displays/                 # 🖥️ 显示设备
│   │   ├── display9341.yaml     # ILI9341显示屏
│   │   └── glyphs.yaml          # 字形配置
│   │
│   └── legacy/                   # 🗄️ 旧版配置
│       └── my8266.yaml          # ESP8266旧版
│
├── resources/                    # 📁 资源文件
│   ├── fonts/                   # 字体文件
│   ├── images/                  # 图片资源  
│   └── static/                  # 静态资源
│
├── test-code/                   # 🧪 测试代码
│   └── test_VOC-CO2-HCHO-Sensor.cpp
│
├── docs/                        # 📋 文档
├── secrets.yaml                 # 敏感配置
├── README.md                    # 主说明文档
└── ORGANIZATION.md              # 详细组织说明
```

## 🚀 快速使用

### 编译配置
```bash
esphome compile configs/environmental-sensors/21voc-esp32c3.yaml
```

### 上传固件  
```bash
esphome run configs/environmental-sensors/21voc-esp32c3.yaml
```

### OTA更新
```bash
esphome run configs/environmental-sensors/21voc-esp32c3.yaml --device OTA
```

## 📋 设备分类

| 分类 | 数量 | 主要设备 |
|------|------|----------|
| 环境传感器 | 8个 | VOC, 温湿度, 气压, 惯性 |
| 雷达传感器 | 3个 | LD2402, LD2410, LD2450 |
| 摄像头云台 | 3个 | ESP32-S3摄像头, PTZ |
| 执行器 | 2个 | 舵机, LED |
| 测量监控 | 1个 | 功率监控 |
| 显示设备 | 2个 | TFT显示屏 |
| 旧版配置 | 1个 | ESP8266 |

## ✨ 整理完成

所有配置文件已按功能分类整理，便于：
- 🔍 快速查找相关配置
- 📝 维护和更新
- 🚀 新项目开发  
- 📚 技术文档管理

查看 `ORGANIZATION.md` 获取详细说明。
