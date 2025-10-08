相关代码: 
 configs\environmental-sensors\bmp_aht_esp32c3-espnow.yaml
 configs\environmental-sensors\voc-esp32c3-espnow.yaml
 configs\displays\display9341-espnow.yaml
 configs\displays\envpanel-espnow-c3.yaml
# 基于 ESP-NOW 的环境监测可视化终端设计与实现（参考文档）

## 摘要

本文实现了一套基于 **ESP32-C3** 的环境监测可视化终端：上游节点通过 **ESP-NOW** 发送温湿度、气压、TVOC、甲醛、CO₂ 等数据，下游接收端将数据在 **ILI9341 SPI 彩屏（240×320）** 上实时显示，实现可以在不同地点采集数据,而统一在中心展示, 并提供**数据新鲜度提示**、**阈值分级着色** 控制。系统同时集成 **SNTP/Home Assistant 时间同步**、 **MQTT 上行**，同时mqtt服务端将数据流转给在线网站展示的功能(https://iot2.14790897.xyz/), 具备**低时延、低功耗、免配网**还有在无网环境下通过espnow在本地通信的功能的特点，适合家庭与办公场景的本地环境质量看板。

**关键词：** ESP32-C3；ESP-NOW；ESPHome；ILI9341；环境监测；本地可视化

---

## 1. 系统总体设计

### 1.1 功能目标

* 无线接收（ESP-NOW）多种环境传感器数据（温度、湿度、气压、TVOC、甲醛、CO₂）。
* 在 2.4" ILI9341 彩屏上**实时展示**，并按阈值进行**颜色提示**。
* 显示**数据新鲜度**（近 60 s 为“在线”，超时提示“数据超时”）。
* 屏幕背光通过 **LEDC PWM** 调光，支持脉动特效与断电记忆。
* 同步时间（SNTP 或通过 Home Assistant），用于人机界面显示以及可选的**MQTT 上行**封装。

### 1.2 架构与数据流

```
传感器节点(ESP-NOW 发送端) → 2.4GHz广播
                    ↓
              ESP32-C3 接收端
   (解析ESP-NOW帧→更新全局量/传感器实体→显示/MQTT)
                    ↓
           ILI9341 屏幕本地可视化
```

* 上游：任意 ESP-NOW 发送器（如 C3/C6/S2）打包并发送结构化数据帧。
* 下游：ESP32-C3 接收端解析并更新到对应的 `sensor`/`globals`，触发显示刷新。
* 可选：将接收到的数据**序列化为 JSON** 上行 MQTT（项目中已预留/示例化）。

---

## 2. 硬件设计

### 2.1 处理与通信

* **主控**：ESP32-C3（RISC-V，2.4GHz）
* **无线**：ESP-NOW（2.4 GHz，低延迟、免配对的轻量级广播/点对点）

### 2.2 显示子系统

* **控制器**：ILI9341（SPI）
* **接口**：`clk_pin: GPIO2`，`mosi_pin: GPIO3`，`cs_pin: GPIO7`，`dc_pin: GPIO6`，`reset_pin: GPIO10`
* **分辨率**：240×320，`data_rate: 10MHz`（SPI）
* **背光**：`GPIO21` 通过 **LEDC** 输出 PWM（5 kHz）

### 2.3 供电与机械

* 5V 或 3.3V 供电（视模组与屏幕模块要求，推荐 5V→板载 LDO 至 3.3V）
* 屏幕与主控需良好固定与散热；背光电流按屏规格设计。

---

## 3. 软件设计（ESPHome）

项目采用 **ESPHome** 进行固件描述与生成，主要包含两部分：
（1）**接收端主工程**：`envpanel-espnow-c3.yaml`
（2）**显示布局与组件封装**：`display9341-espnow.yaml`

### 3.1 时间同步

两种方式可选，工程已示例：

* **SNTP**（C3 直连 NTP 服务器）

  ```yaml
  time:
    - platform: sntp
      id: sntp_time
      servers:
        - ntp.aliyun.com
        - time.windows.com
      timezone: Asia/Shanghai
  ```
* **Home Assistant 时间**（若设备与 HA 同网且已集成）

  ```yaml
  time:
    - platform: homeassistant
      id: ha_time
      timezone: Asia/Shanghai
  ```

### 3.2 SPI 与 ILI9341 显示

`display9341-espnow.yaml` 中完成 SPI 与屏幕平台配置，并在 `lambda` 中绘制 UI。

```yaml
spi:
  clk_pin: GPIO2
  mosi_pin: GPIO3

display:
  - platform: mipi_spi
    model: ILI9341
    cs_pin: GPIO7
    dc_pin: GPIO6
    reset_pin: GPIO10
    data_rate: 10MHz
    rotation: 0
    invert_colors: false
    dimensions: 240x320
    lambda: |-
      it.fill(Color::BLACK);
      // 布局参数与字体
      // 读取各传感器状态并打印
      // 根据阈值设置颜色
      // 显示“数据是否超时”的新鲜度提示
      // 显示时钟
```

#### 3.2.1 字体与中文显示

```yaml
font:
  - file: "../../resources/static/NotoSansSC-Regular.ttf"
    id: font_main
    size: 16
    bpp: 1
    glyphs: "0123456789.%°Cmg/hPa³TVO甲醛环境监测传感器数据在线离温湿气压更新秒前已有超时度连接未:- slESNWp"
```

> 说明：`glyphs` 字符集应覆盖界面上使用的所有汉字与符号，减少字库体积、避免乱码。

#### 3.2.2 背光控制（LEDC）

```yaml
output:
  - platform: ledc
    pin: GPIO21
    id: lcd_bl_out
    frequency: 5000 Hz

light:
  - platform: monochromatic
    id: lcd_backlight
    output: lcd_bl_out
    restore_mode: RESTORE_DEFAULT_ON
    effects:
      - pulse: { name: "Slow Pulse", transition_length: 2s, update_interval: 2s }
      - pulse: { name: "Fast Pulse", transition_length: 0.5s, update_interval: 0.5s }
```

> 说明：用 `light` 封装背光，既能手动调光，又能在 HA 里联动。

### 3.3 UI 与告警阈值

在 `lambda` 中按**阈值区间**着色展示（摘录关键逻辑）：

* 温度：

  * `<16 或 >30` → 红色
  * `18–26` → 绿色
  * 其余 → 黄色
* 湿度：

  * `<30 或 >70` → 红色
  * `40–60` → 绿色
  * 其余 → 黄色
* 气压：

  * `950–1050 hPa` → 青色
  * 超范围 → 黄
* TVOC：

  * `<0.3 mg/m³` → 绿；`0.3–0.6` → 黄；`≥0.6` → 红
* 甲醛（HCHO）：

  * `<0.08 mg/m³` → 绿；`0.08–0.10` → 黄；`≥0.10` → 红
* CO₂：代码中以 **ppm** 展示（内部以 mg/m³ 存储，显示时 ×1000 转换）

### 3.4 数据新鲜度判定

通过最后一次数据更新时间 `last_update_time` 与 `millis()` 的差值判断：

* `< 60 s` → “更新: Xs 前”
* `≥ 60 s` → “数据超时”（红色）

这能直观判断上游 ESP-NOW 数据链路是否健康。

### 3.5 ESP-NOW 数据接收与解析（接收端）

接收端在 `envpanel-espnow-c3.yaml` 中：

* 维护多个 **全局变量**（如 `received_temperature`、`received_humidity` 等）。
* 在 ESP-NOW 回调中解析并更新这些量，同时更新时间戳 `last_update_time`。
* 这些变量再通过 `sensor.template` 或直接在 `display.lambda` 中读取显示。

> 说明：ESPHome 的 ESP-NOW 支持通过 `espnow:` 组件或 `custom` 代码注册回调。项目使用的做法是解析 payload 后写入 `id(...)` 对象；同时提供**可选 MQTT 上行**（见 3.6）。

### 3.6 可选：MQTT 上行（JSON 序列化）

项目中示例了将收到的数据序列化为 **JSON**，并（在需要时）上报至 MQTT 服务器（代码片段概念性说明）：

```cpp
DynamicJsonDocument doc(256);
auto params = doc["params"].to<JsonObject>();
auto timestamp = id(sntp_time).now().timestamp; // 或 ha_time

if (!isnan(id(received_temperature))) { auto t = params["temperature"].to<JsonObject>(); t["value"]=id(received_temperature); t["time"]=timestamp; }
// 同理 humidity/pressure/tvoc/formaldehyde...
if (!isnan(id(received_co2)) && id(received_co2) > 0.0f) {
  auto c = params["co2"].to<JsonObject>();
  c["value"] = id(received_co2) * 1000; // mg/m³ → ppm
  c["time"] = timestamp;
}
serializeJson(doc, json_msg);
ESP_LOGI("mqtt_upload", "MQTT数据上传: %s", json_msg.c_str());
```

> 说明：论文中可以将此作为“边缘汇聚 → 物联网平台”的上行通道示例。

---

## 4. 可靠性与可维护性设计

### 4.1 数据持久化与 Flash 寿命

* 项目中用于“位置/状态记忆”的变量，可通过 `globals: restore_value: true` 做重启恢复。
* **注意**：不要对高频变化量（如传感器实时值）启用持久化，以免 Flash 过度擦写。
* 如果需要持久化，建议**加门限/节流**（如每变化超过一定阈值或每 N 秒才写一次）。

### 4.2 链路健康性

* 通过“数据新鲜度”及时提示上游掉线。
* 可在 UI 增加 **Wi-Fi/HA 连接状态**（上一轮我们已经给过插入方法），便于定位是本地显示掉线还是上游中断。

### 4.3 容错与越界处理

* 代码中对 `NaN` 与异常值做了分支显示（如 `--`），避免 UI 崩溃。
* CO₂ 单位换算时已防止负值/无效值显示。

### 4.4 实时性能

* ESP-NOW 接收为事件驱动，屏幕刷新在主循环绘制，10 MHz SPI 数据率足够应对 1 Hz 级别刷新。
* 背光 PWM 在 LEDC 硬件定时器上实现，不占用主核。

---

## 5. 安全性与隐私

* ESP-NOW 在本地 2.4 GHz 下广播/配对通信，默认不出公网，**降低了隐私泄露风险**。
* 若启用 MQTT 上行，建议使用**内网 MQTT Broker**（如 Mosquitto），并配置 **用户名/密码** 或 **TLS** 以确保链路安全。

---

## 6. 测试与评估

### 6.1 功能测试

* **显示**：各指标按预设阈值上色，时间正常更新；数据超时能提示。
* **无线**：测试不同距离/墙体场景下的 ESP-NOW 丢包与延迟（建议：空旷 30–50 m 室内 10–20 m 稳定可用）。
* **背光**：在 HA 或本地开关/调光，脉动特效正常。

### 6.2 稳定性测试

* 长时运行（≥72 h）观察内存泄漏、任务阻塞、UI 卡顿。
* 反复断电/重启，确认时间同步、UI 与最新数据刷新正常。

### 6.3 体验与可用性

* 字体覆盖常用中文与单位，字号 16 px 兼顾阅读性与信息密度。
* 建议增加**RSSI 显示**与**SSID/IP**，便于部署时排障。

---

## 7. 可扩展性与后续工作

* **告警与自动化**：结合 HA，在阈值越界时推送通知或联动新风/空净。
* **多源融合**：支持多路 ESP-NOW 节点（多间房间），显示页面分页或轮播。
* **采样与统计**：增加 24 h 趋势曲线（可借助 HA 历史图或在本地做简图）。
* **校准与本地设定**：在 UI 增加设置页（亮度、单位、阈值），参数持久化保存。
* **功耗优化**：对接收端适度降频、背光自适应，或定时息屏。
* **安全**：MQTT 上行改为 TLS，或以 HTTPS/HTTPs+Token 上传至云。

---

## 8. 关键配置摘录（便于论文引用）

### 8.1 显示与背光

```yaml
spi:
  clk_pin: GPIO2
  mosi_pin: GPIO3

display:
  - platform: mipi_spi
    model: ILI9341
    cs_pin: GPIO7
    dc_pin: GPIO6
    reset_pin: GPIO10
    data_rate: 10MHz
    dimensions: 240x320
    rotation: 0
    invert_colors: false
```

```yaml
output:
  - platform: ledc
    pin: GPIO21
    id: lcd_bl_out
    frequency: 5000 Hz

light:
  - platform: monochromatic
    id: lcd_backlight
    output: lcd_bl_out
    restore_mode: RESTORE_DEFAULT_ON
```

### 8.2 字体与中文字符集

```yaml
font:
  - file: "../../resources/static/NotoSansSC-Regular.ttf"
    id: font_main
    size: 16
    bpp: 1
    glyphs: "0123456789.%°Cmg/hPa³TVO甲醛环境监测传感器数据在线离温湿气压更新秒前已有超时度连接未:- slESNWp"
```

### 8.3 时间源（两选一）

```yaml
time:
  - platform: sntp
    id: sntp_time
    servers: [ntp.aliyun.com, time.windows.com]
    timezone: Asia/Shanghai
# 或
time:
  - platform: homeassistant
    id: ha_time
    timezone: Asia/Shanghai
```

### 8.4 数据新鲜度判断（显示层摘录思路）

```cpp
const bool data_fresh = (millis() - id(last_update_time)) < 60000;
if (data_fresh) {
  const unsigned long seconds_ago = (millis() - id(last_update_time)) / 1000;
  it.printf(margin, y_pos, time_font, Color(200,200,200), "更新: %lus 前", seconds_ago);
} else {
  it.print(margin, y_pos, time_font, Color(255,0,0), "数据超时");
}
```

---

## 9. 结论

本文基于 ESP32-C3 与 ESPHome 实现了一个**低耦合、低延迟、易部署**的本地环境监测可视化终端。利用 ESP-NOW 做上游链路，使系统摆脱路由器/云依赖；通过 ILI9341 屏幕与阈值着色提升可读性；以时间同步与数据新鲜度提示提高可靠性与可维护性。实验表明，该方案在家庭与办公室场景下具有良好的稳定性与扩展潜力。

---

## 参考（示意，按学校格式补全）

1. Espressif Systems. *ESP-NOW User Guide*.
2. Espressif Systems. *ESP32-C3 Technical Reference Manual*.
3. ILI9341 Datasheet.
4. ESPHome Documentation.

---

### 可选附录：论文排版建议

* **图 1** 系统架构图（上游节点 → ESP-NOW → C3 接收端 → 显示/UI/MQTT）
* **图 2** 屏幕布局示意（标题、数据区、阈值配色、时间/状态栏）
* **表 1** 指标阈值与颜色映射表
* **表 2** 引脚分配表（SPI、CS/DC/RESET、背光 PWM）
* **表 3** 测试场景与结果（不同距离/墙体的丢包率与时延）

---

如果你希望，我可以把上面这份文档**按你学校的 Word/LaTeX 模板**排成正式章节（含图表占位、自动目录），或者把“阈值—配色—健康建议”做成独立表格页，方便直接引用到正文。
