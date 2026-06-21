---
title: "ESPHome Development Rules"
summary: "ESPHome 开发中的踩坑规则、C++ API 约定和常用模式"
agent_created: true
read_when:
  - Writing or editing ESPHome YAML configurations
  - Debugging ESPHome compilation errors
  - Using deep_sleep, espnow, or other ESP-IDF components in ESPHome
---

# ESPHome 开发规则与约定

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

## 3. API Encryption Key

```yaml
# 正确 — 纯 base64 字符串 (32字节 = 44字符)
api_encryption_key: "AtdLeBiiI5NAxBE7JJxXALvhX4ay8Y2R+SoGUA39qm4="

# 错误 — 不能有 "base64:" 前缀
api_encryption_key: "base64:abcdefghijklmnopqrstuvwxyz123456"
```

生成方法:
```bash
python -c "import secrets, base64; print(base64.b64encode(secrets.token_bytes(32)).decode())"
```

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

### lambda 中调用 script

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
| GPIO12/13 LED 状态 | 灭 (高阻) | 亮 (SPI驱动初始化) |

> ESP-IDF 的 SPI Flash 驱动在启动时会触碰 GPIO12/13（即使 DIO 不用），导致 LED 亮。Arduino 不会。

## 6. 无 WiFi 设备的限制

- `esphome run` 会报 "Found no valid options for upload/logging" — 这是预期行为
- 烧录方式: `esptool.py --port COMx write_flash 0x0 firmware.factory.bin`
- 或: `esphome upload config.yaml --device COMx`
- 无法远程查看日志 (无 API/MQTT)，调试需连串口
- **HA 不可见** — 无 WiFi/API 的设备无法被 HA 发现或添加，只能通过网关间接控制
- 如果设备之前刷过有 WiFi 固件，HA 会保留旧记录（显示离线），需手动删除

## 6.5 Web Server

- 需要 `web_server: port: 80` 才能通过 `http://xxx.local/` 访问设备网页
- `api:` 组件只提供 HA 通信，不提供 HTTP 页面
- 没有 `web_server` 时访问 `.local` 会连接超时

## 7. 合宙 ESP32-C3 开发板 (airm2m_core_esp32c3)

```yaml
esp32:
  board: airm2m_core_esp32c3
  framework:
    type: esp-idf
```

**关键点：Flash DIO 模式**
- 合宙 CORE ESP32-C3 使用外置 SPI Flash，2线 DIO 模式
- GPIO12/GPIO13 硬件上可用（未连 Flash），但 **ESPHome 的 pin 验证器仍会阻止**
- 即使指定了 `board: airm2m_core_esp32c3`，ESPHome 仍报 "pin used by SPI/PSRAM"
- **解决方案**: 绕过 ESPHome GPIO 抽象，直接用 ESP-IDF API 在 lambda 中操作

**板载 LED 关闭方法 (唯一有效方式):**
- D4 = GPIO12, D5 = GPIO13, 高电平有效
- 必须用 ESP-IDF `gpio_set_level()` 在 lambda 中直接控制
- Deep sleep 设备还需 `gpio_hold_en()` + `gpio_deep_sleep_hold_en()` 锁住状态

```yaml
# 无 deep sleep 的设备 (如网关):
esphome:
  on_boot:
    priority: 600
    then:
      - lambda: |-
          gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
          gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
          gpio_set_level(GPIO_NUM_12, 0);
          gpio_set_level(GPIO_NUM_13, 0);

# 有 deep sleep 的设备 (GPIO 会在睡眠时浮空):
esphome:
  on_boot:
    then:
      - lambda: |-
          gpio_hold_dis(GPIO_NUM_12);
          gpio_hold_dis(GPIO_NUM_13);
          gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
          gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
          gpio_set_level(GPIO_NUM_12, 0);
          gpio_set_level(GPIO_NUM_13, 0);
          gpio_hold_en(GPIO_NUM_12);
          gpio_hold_en(GPIO_NUM_13);
          gpio_deep_sleep_hold_en();
```

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
