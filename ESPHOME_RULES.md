# ESPHome 开发规则与约定

> 本项目开发中积累的踩坑规则、C++ API 约定和常用模式。

## 1. Entity 命名规则

- **`name` 字段只能用英文** (ASCII)
- 纯中文名会转为相同的 ASCII ID (如 `____`) 导致 "Duplicate entity" 冲突
- `friendly_name` (设备级) 可以用中文，但实体 `name` 必须英文
- 正确: `name: "Door Open"` / 错误: `name: "门锁开门"`

## 2. Deep Sleep C++ API

在 lambda 中操作 deep_sleep 组件:

```cpp
// 正确 — 公开方法
id(deep_sleep_ctrl).prevent_deep_sleep();
id(deep_sleep_ctrl).allow_deep_sleep();
id(deep_sleep_ctrl).set_sleep_duration(ms);
id(deep_sleep_ctrl).begin_sleep(true);  // 手动进入深睡

// 错误 — prevent_ 是 protected 成员，不可直接访问
id(deep_sleep_ctrl).prevent();   // 编译错误!
id(deep_sleep_ctrl).prevent_ = true;  // 编译错误!
```

- 源码参考: `esphome/components/deep_sleep/deep_sleep_component.h`
- YAML action `deep_sleep.prevent` / `deep_sleep.allow` / `deep_sleep.enter` 对应以上方法


## 4. ESP-NOW + Deep Sleep 模式

### 架构

```
[HA] ←WiFi/API→ [网关 ESP32] ←ESP-NOW→ [门锁 ESP32-C3 (电池)]
                 (有线供电)              (周期性唤醒监听)
```

### 关键配置

**接收端 (无 WiFi, 电池供电):**
- `framework: type: esp-idf` (ESP-NOW 必须)
- 不包含 `wifi:` / `api:` / `ota:` 组件
- `espnow:` 需设置 `channel` (必须匹配网关 WiFi AP 信道)
- `deep_sleep:` 设 `run_duration` + `sleep_duration`
- OTA 只能 USB 烧录

**发送端 (有线供电, 连 HA):**
- `wifi:` + `api:` + `ota:` 正常配置
- `espnow:` 的 channel 自动跟随 WiFi，不能手动设置
- 重试策略: 发送持续时间 > 接收端一个完整周期 (run + sleep)

### 重试策略计算

```
接收端周期 = run_duration + sleep_duration
发送端需要覆盖时间 ≥ 接收端周期
示例: run=200ms, sleep=1000ms → 周期=1200ms
      发送: repeat 10 × (send + delay 150ms) = 1500ms > 1200ms ✓
```

### lambda 中调用

```cpp
// 在 espnow on_receive lambda 中触发 script
id(my_script).execute();

// 在 lambda 中触发 button
id(my_button).press();
```

## 5. ESP-IDF vs Arduino 框架

| 功能 | Arduino | ESP-IDF |
|------|---------|---------|
| ESP-NOW | 不支持 | 支持 |
| BLE | 部分 | 完整 |
| Deep Sleep + ESP-NOW | 不支持 | 支持 |
| 编译速度 | 快 | 慢 |
| OTA (无WiFi时) | N/A | USB only |

## 6. 无 WiFi 设备的限制

- `esphome run` 会报 "Found no valid options for upload/logging" — 这是预期行为
- 烧录方式: `esptool.py --port COMx write_flash 0x0 firmware.factory.bin`
- 或: `esphome upload config.yaml --device COMx`
- 无法远程查看日志 (无 API/MQTT)，调试需连串口

## 7. 合宙 ESP32-C3 开发板 (airm2m_core_esp32c3)

```yaml
esp32:
  board: airm2m_core_esp32c3
  framework:
    type: esp-idf
```

**关键：Flash DIO 模式**
- 合宙 CORE ESP32-C3 使用外置 SPI Flash，2线 DIO 模式
- GPIO12/GPIO13 在 DIO 模式下**可用**作普通 GPIO（板载 LED D4/D5）
- 如果不指定 board 或使用默认 QIO 模式，ESPHome 会报 "pin used by SPI/PSRAM"
- PlatformIO 板子定义位置: `.pio-core-esphome/platforms/espressif32/boards/airm2m_core_esp32c3.json`
- 关键配置: `"flash_mode": "dio"`

**板载 LED：**
- D4 = GPIO12, 高电平有效
- D5 = GPIO13, 高电平有效
- 关闭方法: `switch` + `restore_mode: ALWAYS_OFF`

**SPI Flash 引脚 (不可用)：**
- GPIO14 = FLASH_CS
- GPIO15 = FLASH_CK
- GPIO16 = FLASH_D0
- GPIO17 = FLASH_D1

**其他注意：**
- GPIO9 = BOOT 键 (strapping pin, 上电时不能下拉)
- GPIO11 = 默认为 SPI Flash VDD, 需烧 efuse 解锁才能当 GPIO
- GPIO18/19 = USB (新款 USB 直连版本被占用)

## 8. MAC 地址获取

```bash
# 连接 USB 后
esptool.py --port COMx read_mac

# 或首次烧录 ESPHome 固件时串口会输出 MAC
# 格式: XX:XX:XX:XX:XX:XX (6字节, 冒号分隔)
```
