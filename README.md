<!-- 
## ⚠️ 已知问题

### 传感器死机问题
**现象描述**：
- 传感器可能会出现死机状态
- 死机时会持续发送固定数值：
  - **TVOC: 5.000 mg/m³** (长时间不变)
  - **CO₂: 5.000 mg/m³** (长时间不变)
  - 甲醛数值可能正常变化或也固定

**识别方法**：
1. TVOC和CO₂长时间保持在5.000 mg/m³不变
2. 即使环境条件变化，数值也不响应
3. 甲醛可能仍有小幅变化，但TVOC/CO₂完全静止

**解决方案**：
1. **重启ESP32设备** - 最简单有效的方法
2. **断电重启传感器** - 如果ESP32重启无效
3. **检查连接** - 确保UART连接稳定
4. **环境检查** - 确保传感器工作环境温湿度适宜

**预防措施**：
- 定期监控数值变化
- 设置自动重启机制（可选）
- 避免传感器长时间工作在极端环境

## 配置文件
- **主配置**: `voc-esp32c3.yaml`
- **参考代码**: `test_VOC-CO2-HCHO-Sensor.cpp` -->

## ✨ 新功能: MQTT响应接收

### 📡 ESP-NOW环境监测面板 MQTT功能

`envpanel-espnow-c3.yaml` 现已支持MQTT响应信息获取功能！

**核心功能**:
- ✅ 实时接收物联网平台的MQTT响应消息
- ✅ 每60秒自动上传传感器数据到物联网平台
- ✅ 通过text_sensor显示MQTT状态和响应内容
- ✅ 详细的连接状态和响应时间监控

**快速开始**:
```bash
# 1. 复制配置模板
cp secrets.yaml.example secrets.yaml

# 2. 编辑secrets.yaml填写MQTT配置

# 3. 编译上传
esphome run configs/displays/envpanel-espnow-c3.yaml
```

**文档**:
- 📖 [详细使用指南](docs/MQTT_RESPONSE_GUIDE.md)
- 🚀 [快速开始](docs/MQTT_QUICK_START.md)

---

## 常用命令
```bash
# 编译固件
esphome compile voc-esp32c3.yaml

# 上传固件 (OTA)
esphome upload voc-esp32c3.yaml --device OTA

# 查看日志
esphome logs voc-esp32c3.yaml

# 运行 (编译+上传+日志)
esphome run voc-esp32c3.yaml

# SHT30传感器 (旧配置)
esphome run sht30-sensor.yaml
esphome run sht30-esp32c3.yaml --device OTA
esphome run voc-esp32c3.yaml --device OTA
esphome run cam-esp32s3.yaml --device OTA
esphome run bmp_aht_esp32c3.yaml --device OTA
esphome run radar-esp32c3.yaml --device OTA
esphome run led-esp32c3.yaml --device COM33
esphome run servo-esp32c3.yaml --device OTA
esphome run ld2402-esp32c3.yaml --device OTA
esphome run PTZ-esp32c3.yaml --device OTA
esphome run bmi160-esp32c3.yaml --device OTA
esphome run esp32s3-n8.yaml --device OTA
esphome run ina226-esp32c3.yaml --device OTA
esphome run ld2410-esp32c3.yaml --device OTA
那个注意一下啊，esp32c3的12,13是它的led的，所以你在这里输入高电频的话，那个led会闪烁