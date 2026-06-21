# ESP-NOW + Deep Sleep 门锁方案实战：从 7 天续航到 1 个月+

> 2026-06-21 | ESPHome | ESP32-C3 | 低功耗 | ESP-NOW

## 背景：WiFi 常连的续航困境

我的智能门锁基于 ESP32-C3 + MG946R 舵机，通过 Home Assistant 远程控制。原方案使用 WiFi 常连接：

- 活跃电流：~95mA（WiFi LIGHT 省电模式）
- 夜间深睡 10h（22:00-08:00），其余 14h 活跃
- **10000mAh 电池续航：约 7 天**

7 天换一次电，不能忍。

## 方案选型：为什么是 ESP-NOW？

| 方案 | 唤醒连接耗时 | 单次监听功耗 | 响应延迟 | 远程控制 |
|------|-------------|-------------|---------|---------|
| WiFi 常连 | - | ~95mA 持续 | 即时 | HA 直连 |
| BLE 唤醒 | ~50ms | 低 | 即时 | 需网关 |
| **ESP-NOW 轮询** | **0ms (无需连接)** | **~80mA × 30ms** | **≤1-2s** | **经网关** |

ESP-NOW 的核心优势：**无需握手连接**。唤醒后直接收发，整个监听窗口可以压到毫秒级。

## 最终架构

```
Home Assistant
     ↕ WiFi + API
[ESP32-C3 网关] ←── 有线供电，门旁边
     ↕ ESP-NOW
[ESP32-C3 门锁] ←── 电池供电，周期性唤醒
```

- **网关**：始终在线，连 WiFi + HA，收到 HA 指令后通过 ESP-NOW 转发
- **门锁**：无 WiFi，每 1s 醒来监听 200ms，收到命令驱动舵机

## ESPHome 实现

### 门锁端 (door-espnow.yaml)

```yaml
esp32:
  board: airm2m_core_esp32c3
  framework:
    type: esp-idf  # ESP-NOW 必须用 ESP-IDF

# 无 WiFi、无 API、无 OTA

espnow:
  channel: 1  # 必须与网关 WiFi AP 信道一致
  peers:
    - "9C:13:9E:73:88:F4"  # 网关 MAC
  on_receive:
    then:
      - lambda: |-
          if (size < 1 || id(cmd_handled)) return;
          id(cmd_handled) = true;
          if (data[0] == 0x01) {
            id(deep_sleep_ctrl).prevent_deep_sleep();
            id(door_open_btn).press();
          }

deep_sleep:
  id: deep_sleep_ctrl
  run_duration: 200ms   # 唤醒监听窗口
  sleep_duration: 1s    # 睡眠时长 = 最大响应延迟
```

### 网关端 (door-gateway.yaml)

```yaml
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

api:
  encryption:
    key: !secret api_encryption_key

espnow:
  peers:
    - "34:CD:B0:A7:BC:00"  # 门锁 MAC

# 重试策略：持续发送 > 门锁一个完整唤醒周期
script:
  - id: send_open_cmd
    mode: restart
    then:
      - repeat:
          count: 10
          then:
            - espnow.send:
                address: "34:CD:B0:A7:BC:00"
                data: [0x01]
            - delay: 150ms
```

网关每 150ms 发一次命令，连发 10 次 (1.5s)，保证覆盖门锁的 1.2s 唤醒周期。

## 踩坑记录

### 坑 1：Deep Sleep 的 C++ API

```cpp
// 错误 - prevent_ 是 protected 成员
id(deep_sleep_ctrl).prevent();  // 编译失败!

// 正确 - 公开方法名是 prevent_deep_sleep()
id(deep_sleep_ctrl).prevent_deep_sleep();
id(deep_sleep_ctrl).allow_deep_sleep();
```

ESPHome 源码里 `prevent_` 是 protected bool，公开方法是 `prevent_deep_sleep()` / `allow_deep_sleep()`。

### 坑 2：Entity 名字只能英文

```yaml
# 错误 - 纯中文转 ASCII ID 全变成 "____"，导致重复冲突
name: "门锁开门"
name: "门锁关门"  # Duplicate entity!

# 正确
name: "Door Open"
name: "Door Close"
```

### 坑 3：合宙 ESP32-C3 板子配置

```yaml
esp32:
  board: airm2m_core_esp32c3  # 关键！设置 DIO flash 模式
  framework:
    type: esp-idf
```

不指定 board 时，ESPHome 默认 QIO 模式，会占用 GPIO12/13（报 "pin used by SPI/PSRAM"）。合宙板用 DIO 模式，GPIO12/13 实际空闲。

但即使指定了 board，**ESPHome 的 pin 验证器仍然拦截** GPIO12/13。解决方案：绕过 ESPHome 抽象层，直接用 ESP-IDF API。

### 坑 4：ESP-IDF 框架下 LED 常亮

同一块合宙板：
- Arduino 框架 → GPIO12/13 LED **不亮**（高阻态）
- ESP-IDF 框架 → GPIO12/13 LED **常亮**（SPI 驱动初始化触碰）

解决方案（无 deep sleep 设备）：

```yaml
on_boot:
  priority: 600
  then:
    - lambda: |-
        gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
        gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_NUM_12, 0);
        gpio_set_level(GPIO_NUM_13, 0);
```

### 坑 5：Deep Sleep 期间 GPIO 浮空

ESP32-C3 的 GPIO12/13 **不是 RTC GPIO**（只有 GPIO0-5 是）。进入 deep sleep 后，非 RTC GPIO 状态丢失，`gpio_hold_en()` 无法锁住。

**结论：软件无解，只能硬件拆 LED。**

| GPIO 范围 | 类型 | Deep Sleep hold |
|-----------|------|-----------------|
| GPIO 0-5 | RTC GPIO | ✅ 有效 |
| GPIO 6-21 | 数字 GPIO | ❌ 浮空 |

### 坑 6：API 加密密钥格式

```yaml
# 错误 - 不能有 "base64:" 前缀
api_encryption_key: "base64:abcdefghijklmnopqrstuvwxyz123456"

# 正确 - 纯 base64 字符串 (32字节)
api_encryption_key: "abcdefghijklmnopqrstuvwxyz123456"
```

生成：`python -c "import secrets,base64;print(base64.b64encode(secrets.token_bytes(32)).decode())"`

## 功耗实测分析

| 参数 | 值 |
|------|---|
| run_duration | 200ms |
| sleep_duration | 1s |
| 周期 | 1200ms |
| 活跃占空比 | 16.7% |
| 估算平均电流 | ~13mA |
| 10000mAh 续航 | **~31 天** |

相比原来的 7 天，提升了 **4.4 倍**。

如果用裸 ESP-IDF（非 ESPHome），启动时间可压到 20ms，占空比降到 2%，理论续航可达 **170 天**。ESPHome 的框架启动开销（~150ms）是主要瓶颈。

## 总结

| | WiFi 方案 | ESP-NOW 方案 |
|---|---|---|
| 平均电流 | ~95mA (14h活跃) | ~13mA |
| 10000mAh 续航 | 7 天 | 31 天 |
| 响应延迟 | 即时 | ≤1.2s |
| OTA 更新 | 无线 | USB |
| 额外硬件 | 无 | 网关 (~15元) |
| 复杂度 | 低 | 中 |

ESP-NOW 方案用一个 15 元的网关换来 4x 续航提升，且响应延迟仅 1 秒左右，对门锁这种低频操作完全可接受。如果未来需要更极致的续航，可以考虑迁移到裸 ESP-IDF 框架，或者加大 sleep_duration 到 2s（续航翻倍，延迟也翻倍）。

## 文件结构

```
configs/actuators/
├── door.yaml           # 原 WiFi 方案 (保留)
├── door-espnow.yaml    # ESP-NOW 门锁接收端
└── door-gateway.yaml   # ESP-NOW 网关发射端
```

## 参考

- [ESPHome ESP-NOW 组件文档](https://esphome.io/components/espnow/)
- [ESPHome Deep Sleep 文档](https://esphome.io/components/deep_sleep/)
- [合宙 ESP32-C3 开发板](https://wiki-zh.luatos.org/chips/esp32c3/board.html)
- [ESP-IDF GPIO Hold API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/peripherals/gpio.html)
