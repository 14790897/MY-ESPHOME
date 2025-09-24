# 📦 PlatformIO 平台版本管理指南

## 🎯 使用本地已安装的平台版本

### 问题描述
ESPHome 默认会下载最新版本的平台和工具链，即使本地已经安装了对应版本，导致：
- 重复下载浪费时间和带宽
- 占用额外存储空间
- 版本不一致可能导致兼容性问题

### ✅ 解决方案

#### 1. **检查本地已安装的平台版本**
```bash
pio platform list
```

#### 2. **在配置文件中指定版本**
```yaml
esphome:
  name: your-device-name
  platformio_options:
    platform: espressif32@6.9.0  # 指定具体版本号
```

#### 3. **常用平台版本对应关系**
| ESPHome 版本 | 推荐 espressif32 版本 | 说明 |
|-------------|---------------------|------|
| 2025.9.x | 6.9.0 | 当前稳定版本 |
| 2025.7.x | 6.4.0 | 较早稳定版本 |
| 自定义构建 | 53.3.13+ | PicoArduino 版本 |

### 🔧 配置模板

#### 标准ESP32配置
```yaml
esphome:
  name: device-name
  platformio_options:
    platform: espressif32@6.9.0
    
esp32:
  board: airm2m_core_esp32c3
  framework:
    type: arduino
```

#### 高级配置（指定工具链）
```yaml
esphome:
  name: device-name
  platformio_options:
    platform: espressif32@6.9.0
    platform_packages:
      - toolchain-riscv32-esp@8.4.0+2021r2-patch5
```

### 📋 最佳实践

1. **统一版本管理**
   - 在项目中使用相同的平台版本
   - 创建 `common.yaml` 包含平台配置

2. **版本固定**
   ```yaml
   # common.yaml
   esphome_common:
     platformio_options:
       platform: espressif32@6.9.0
   ```

3. **包含方式**
   ```yaml
   # 设备配置文件
   packages:
     common: !include ../../common.yaml
   ```

### 🚨 注意事项

- 指定版本后，ESPHome 不会自动更新平台
- 确保指定的版本与 ESPHome 版本兼容
- 删除 `.esphome/build` 目录可强制重新编译

### 📊 当前本地可用版本
根据您的系统，可用版本包括：
- `espressif32@6.9.0` ✅ 推荐
- `espressif32@6.4.0` ✅ 稳定
- `espressif32@4.4.0` ❓ 较旧版本
- 自定义版本 `53.3.13`, `54.3.21` 🔧 特殊用途

### 💡 临时解决方案
如果不想修改每个配置文件，可以设置环境变量：
```bash
$env:PLATFORMIO_DEFAULT_PLATFORM_ESPRESSIF32 = "6.9.0"
```