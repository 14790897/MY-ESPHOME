#include "esphome.h"

class SHT30UART : public Component, public UARTDevice {
 public:
  SHT30UART(UARTComponent *parent, Sensor *temperature_sensor, Sensor *humidity_sensor)
    : UARTDevice(parent), temperature_sensor_(temperature_sensor), humidity_sensor_(humidity_sensor) {}

  void setup() override {
    // 初始化设置
    ESP_LOGI("sht30_uart", "SHT30 UART传感器初始化完成");
  }

  void loop() override {
    // 读取UART数据
    while (available()) {
      char c = read();
      if (c == '\n' || c == '\r') {
        if (!buffer_.empty()) {
          process_line(buffer_);
          buffer_.clear();
        }
      } else {
        buffer_ += c;
        // 防止缓冲区过长
        if (buffer_.length() > 100) {
          buffer_.clear();
        }
      }
    }
  }

 private:
  Sensor *temperature_sensor_;
  Sensor *humidity_sensor_;
  std::string buffer_;

  void process_line(const std::string &line) {
    // 期望格式：R:070.0RH 032.4C
    ESP_LOGD("sht30_uart", "收到数据: %s", line.c_str());

    if (line.rfind("R:", 0) == 0 && line.length() >= 17) {
      try {
        float hum = std::stof(line.substr(2, 5));   // 070.0
        float temp = std::stof(line.substr(11, 5)); // 032.4

        ESP_LOGI("sht30_uart", "解析成功 - 湿度: %.1f%%, 温度: %.1f°C", hum, temp);

        if (humidity_sensor_ != nullptr) {
          humidity_sensor_->publish_state(hum);
        }
        if (temperature_sensor_ != nullptr) {
          temperature_sensor_->publish_state(temp);
        }
      } catch (...) {
        ESP_LOGW("sht30_uart", "解析失败: %s", line.c_str());
      }
    } else {
      ESP_LOGD("sht30_uart", "数据格式不匹配: %s", line.c_str());
    }
  }
};
