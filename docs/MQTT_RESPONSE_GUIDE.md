# 📡 MQTT响应信息获取指南

本指南说明如何在 `envpanel-espnow-c3.yaml` 配置中获取和处理MQTT响应信息。

## 🌟 功能概述

### 实现的功能

1. **MQTT连接状态监控** - 实时监控MQTT连接状态
2. **响应消息接收** - 订阅并接收来自物联网平台的响应
3. **响应时间追踪** - 记录最后一次收到响应的时间
4. **状态显示** - 通过文本传感器显示MQTT状态和响应内容

## 📋 配置说明

### 1. MQTT基础配置

```yaml
mqtt:
  broker: !secret mqtt_broker
  username: !secret mqtt_username
  password: !secret mqtt_password
  topic_prefix: !secret mqtt_topic_prefix
  discovery: false
```

### 2. 连接状态回调

#### 连接成功事件
```yaml
on_connect:
  - lambda: |-
      ESP_LOGI("mqtt", "MQTT已连接到物联网平台");
      id(mqtt_response).publish_state("MQTT连接成功");
      id(mqtt_last_response_time) = millis();
```

#### 连接断开事件
```yaml
on_disconnect:
  - lambda: |-
      ESP_LOGW("mqtt", "MQTT连接断开");
      id(mqtt_response).publish_state("MQTT断开连接");
```

### 3. 消息订阅与处理

#### 属性设置消息订阅
订阅物联网平台发送的属性设置命令：
```yaml
on_message:
  - topic: !secret mqtt_topic_property_set
    then:
      - lambda: |-
          ESP_LOGI("mqtt", "收到MQTT属性设置: %s", x.c_str());
          id(mqtt_response).publish_state(x.c_str());
          id(mqtt_last_response_time) = millis();
```

#### 属性上报响应订阅
使用通配符订阅属性上报的响应消息：
```yaml
  - topic: +/thing/property/post_reply
    then:
      - lambda: |-
          ESP_LOGI("mqtt", "收到属性上报响应: %s", x.c_str());
          id(mqtt_response).publish_state(("上报响应: " + x).c_str());
          id(mqtt_last_response_time) = millis();
```

> **注意**: 使用 `+` 通配符可以匹配单级主题，适用于不同设备的响应主题。

### 4. 数据上传

每60秒自动上传传感器数据到物联网平台：
```yaml
interval:
  - interval: 60s
    then:
      - mqtt.publish:
          topic: !secret mqtt_topic_property_post
          payload: !lambda |-
            // JSON格式数据构建
            char json_buffer[512];
            // ... 构建包含所有传感器数据的JSON
            return std::string(json_buffer);
```

## 📊 状态传感器

### MQTT Response 传感器
显示最近收到的MQTT响应内容：
```yaml
- platform: template
  name: "MQTT Response"
  id: mqtt_response
  icon: "mdi:message-reply-text"
```

**输出示例**:
- `MQTT连接成功`
- `上报响应: {"code":200,"message":"success"}`
- `{"action":"set_backlight","value":80}`

### MQTT Status 传感器
显示MQTT连接和响应状态：
```yaml
- platform: template
  name: "MQTT Status"
  id: mqtt_status
  icon: "mdi:cloud-check"
  lambda: |-
    if (id(mqtt_last_response_time) == 0) {
      return std::string("等待响应");
    }
    unsigned long age = (millis() - id(mqtt_last_response_time)) / 1000;
    if (age < 120) {
      return std::string("已连接");
    } else if (age < 600) {
      return std::string("响应延迟 " + to_string(age) + " 秒");
    } else {
      return std::string("响应超时");
    }
  update_interval: 10s
```

**状态说明**:
- `等待响应` - 还未收到任何MQTT响应
- `已连接` - 最近2分钟内收到过响应
- `响应延迟 X 秒` - 2-10分钟内收到过响应
- `响应超时` - 超过10分钟未收到响应

## 🔧 使用方法

### 1. 配置 secrets.yaml

创建或更新 `secrets.yaml` 文件（与配置文件在同一目录）：

```yaml
# WiFi配置
wifi_ssid: "你的WiFi名称"
wifi_password: "你的WiFi密码"

# OTA配置
ota_password: "你的OTA密码"

# MQTT配置
mqtt_broker: "你的MQTT服务器地址"
mqtt_username: "你的用户名"
mqtt_password: "你的MQTT密码"
mqtt_topic_device_name: "你的设备名称"
mqtt_topic_prefix: "$sys/你的用户名/你的设备名称"
mqtt_topic_property_post: "$sys/你的用户名/你的设备名称/thing/property/post"
mqtt_topic_property_set: "$sys/你的用户名/你的设备名称/thing/service/property/set"
```

### 2. 编译和上传

```bash
# 验证配置
esphome config configs/displays/envpanel-espnow-c3.yaml

# 编译固件
esphome compile configs/displays/envpanel-espnow-c3.yaml

# 上传到设备（首次使用USB）
esphome run configs/displays/envpanel-espnow-c3.yaml

# 后续使用OTA更新
esphome run configs/displays/envpanel-espnow-c3.yaml --device OTA
```

### 3. 查看日志

```bash
esphome logs configs/displays/envpanel-espnow-c3.yaml
```

## 📝 日志输出示例

### 连接成功
```
[12:34:56][I][mqtt:123]: MQTT已连接到物联网平台
```

### 收到属性设置命令
```
[12:35:00][I][mqtt:123]: 收到MQTT属性设置: {"brightness":80}
```

### 收到上报响应
```
[12:35:10][I][mqtt:123]: 收到属性上报响应: {"code":200,"msg":"success"}
```

### 数据上传
```
[12:36:00][I][mqtt:123]: 上传数据到物联网平台: {"id":"123456","version":"1.0","params":{"temperature":23.5,"humidity":65.2,...}}
```

## 🔍 故障排查

### 问题1: 未收到MQTT响应

**可能原因**:
1. MQTT服务器未配置响应主题
2. 订阅主题不正确
3. 网络连接问题

**解决方案**:
1. 检查物联网平台是否支持响应消息
2. 确认订阅主题与平台文档一致
3. 查看日志确认MQTT连接状态

### 问题2: MQTT连接失败

**可能原因**:
1. 服务器地址或端口错误
2. 用户名/密码不正确
3. 网络防火墙阻止

**解决方案**:
1. 验证 `secrets.yaml` 中的配置信息
2. 使用MQTT客户端工具测试连接
3. 检查网络和防火墙设置

### 问题3: 响应状态显示超时

**可能原因**:
1. 物联网平台没有发送响应
2. 订阅主题不匹配
3. 数据上传失败

**解决方案**:
1. 在平台查看设备状态和消息记录
2. 调整订阅主题的通配符模式
3. 检查上传数据格式是否正确

## 🎯 高级功能

### 自定义响应处理

你可以在 `on_message` 的 lambda 中添加自定义逻辑来处理不同类型的响应：

```yaml
on_message:
  - topic: !secret mqtt_topic_property_set
    then:
      - lambda: |-
          ESP_LOGI("mqtt", "收到消息: %s", x.c_str());
          
          // 解析JSON响应
          if (x.find("\"brightness\"") != std::string::npos) {
            // 处理亮度调节命令
            ESP_LOGI("mqtt", "收到亮度调节命令");
          }
          
          // 更新响应传感器
          id(mqtt_response).publish_state(x.c_str());
          id(mqtt_last_response_time) = millis();
```

### 添加更多订阅主题

如需订阅更多主题，在 `on_message` 中添加更多条目：

```yaml
on_message:
  - topic: !secret mqtt_topic_property_set
    then: [...]
  
  - topic: +/thing/property/post_reply
    then: [...]
  
  - topic: +/thing/event/post_reply
    then:
      - lambda: |-
          ESP_LOGI("mqtt", "收到事件响应: %s", x.c_str());
          // 处理事件响应
```

## 📚 相关资源

- [ESPHome MQTT组件文档](https://esphome.io/components/mqtt.html)
- [物联网平台MQTT接口文档](请参考你的平台文档)
- [项目主配置文件](../configs/displays/envpanel-espnow-c3.yaml)
- [配置示例](../secrets.yaml.example)

## 💡 提示

1. **保护敏感信息**: 永远不要将 `secrets.yaml` 提交到代码仓库
2. **测试连接**: 在部署前使用MQTT客户端工具测试连接
3. **监控日志**: 定期查看设备日志以发现潜在问题
4. **版本控制**: 保留配置文件的备份以便回滚

---

**最后更新**: 2025年10月5日  
**维护者**: ESP32C3环境监测面板项目组
