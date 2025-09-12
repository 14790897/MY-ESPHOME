/**
 * @file test_VOC-CO2-HCHO-Sensor.cpp
 * @brief 三合一传感器测试程序
 * @description 专门用于测试三合一传感器的独立程序
 *
 * 传感器功能：
 * - TVOC浓度检测 (mg/m³)
 * - 甲醛(CH₂O)浓度检测 (mg/m³)
 * - CO₂浓度检测 (mg/m³)
 *
 * 硬件连接：
 * - RX引脚: GPIO 1 (接传感器TX)
 * - TX引脚: GPIO 0 (接传感器RX)
 * - 波特率: 9600
 * - 数据格式: 8N1
 *
 * 数据协议：
 * - 模块地址: 0x2C
 * - 功能码: 0xE4
 * - 数据长度: 9字节
 * - 校验和: B1+B2+...+B8的低8位
 *
 * @author ESP32C3 Sensor Framework
 * @date 2024
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include <Preferences.h>

// ===== 21VOC传感器配置 =====
#define UART_RX_PIN 1           // UART接收引脚 (GPIO1) - 接传感器TX
#define UART_TX_PIN 0           // UART发送引脚 (GPIO0) - 接传感器RX
#define UART_BAUD_RATE 9600     // 波特率
#define READ_INTERVAL 2000      // 读取间隔 (毫秒)

// ===== 数据质量评估标准 =====
// TVOC浓度等级 (mg/m³)
#define TVOC_EXCELLENT 0.3      // 优秀
#define TVOC_GOOD 0.6          // 良好
#define TVOC_MODERATE 1.0      // 一般
#define TVOC_POOR 3.0          // 较差

// 甲醛浓度等级 (mg/m³)
#define HCHO_SAFE 0.08         // 安全
#define HCHO_ACCEPTABLE 0.10   // 可接受
#define HCHO_CONCERNING 0.12   // 需关注
#define HCHO_DANGEROUS 0.15    // 危险

// CO₂浓度等级 (mg/m³)
#define CO2_FRESH 0.7          // 新鲜空气
#define CO2_ACCEPTABLE 1.0     // 可接受
#define CO2_DROWSY 1.8         // 令人困倦
#define CO2_STUFFY 2.7         // 闷热

// ===== 全局对象和变量 =====
HardwareSerial sensorSerial(1);  // 使用UART1
Preferences preferences;

// VOC-CO2-HCHO传感器数据结构
struct VOCData {
  float tvoc_mgm3;        // TVOC浓度 (mg/m3)
  float ch2o_mgm3;        // 甲醛浓度 (mg/m3)
  float co2_mgm3;         // CO₂浓度 (mg/m3)
  bool valid;             // 数据有效性
  unsigned long timestamp; // 时间戳
};

// 统计数据结构
struct SensorStats {
  // TVOC统计
  float tvoc_sum;
  float tvoc_min;
  float tvoc_max;

  // 甲醛统计
  float hcho_sum;
  float hcho_min;
  float hcho_max;

  // CO₂统计
  float co2_sum;
  float co2_min;
  float co2_max;

  // 计数器
  uint32_t valid_readings;
  uint32_t total_readings;
  uint32_t error_count;
};

// 全局变量
VOCData lastReading;
SensorStats stats;
unsigned long lastReadTime = 0;
unsigned long lastStatsTime = 0;
const unsigned long STATS_INTERVAL = 30000; // 30秒统计间隔

// ===== 函数声明 =====
void initUART();
void initStats();
bool readVOCSensor(VOCData &data);
void parseVOCData(uint8_t* buffer, int length, VOCData &data);
bool validateVOCData(const VOCData &data);
void printVOCReading(const VOCData &data);
void updateStatistics(const VOCData &data);
void printStatistics();
void printDataQualityAssessment(const VOCData &data);
String getTVOCQualityLevel(float tvoc);
String getHCHOSafetyLevel(float hcho);
String getCO2ComfortLevel(float co2);
void debugUARTConnection();
void performSensorTest();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("========================================");
  Serial.println("🌿 VOC-CO2-HCHO三合一传感器测试程序");
  Serial.println("========================================");
  Serial.println("传感器功能:");
  Serial.println("  📊 TVOC浓度检测 (mg/m³)");
  Serial.println("  🏠 甲醛(CH₂O)浓度检测 (mg/m³)");
  Serial.println("  💨 CO₂浓度检测 (mg/m³)");
  Serial.println("========================================");
  Serial.printf("硬件连接:\n");
  Serial.printf("  RX引脚: GPIO %d (接传感器TX)\n", UART_RX_PIN);
  Serial.printf("  TX引脚: GPIO %d (接传感器RX)\n", UART_TX_PIN);
  Serial.printf("  波特率: %d\n", UART_BAUD_RATE);
  Serial.println("========================================");

  // 初始化UART通信
  initUART();

  // 初始化统计数据
  initStats();

  // 初始化Preferences
  if (preferences.begin("21voc_test", false)) {
    Serial.println("✅ Preferences存储初始化成功");
  } else {
    Serial.println("⚠️ Preferences存储初始化失败");
  }

  Serial.println("🚀 系统初始化完成，开始数据采集...");
  Serial.println("💡 提示: 传感器需要预热2-3分钟才能获得稳定读数");
  Serial.println("========================================");

  delay(2000); // 等待传感器稳定
}

void loop() {
  unsigned long currentTime = millis();

  // 定时读取传感器数据
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;

    Serial.println();
    Serial.println("========================================");
    Serial.printf("⏰ [%lu ms] 开始读取21VOC传感器数据\n", currentTime);

    // 检查UART缓冲区状态
    int availableBytes = sensorSerial.available();
    if (availableBytes > 0) {
      Serial.printf("📡 UART缓冲区可用字节数: %d\n", availableBytes);
    }

    VOCData currentReading;
    stats.total_readings++;

    // 读取传感器数据
    if (readVOCSensor(currentReading)) {
      if (currentReading.valid && validateVOCData(currentReading)) {
        lastReading = currentReading;
        stats.valid_readings++;

        // 打印读数
        printVOCReading(currentReading);

        // 打印数据质量评估
        printDataQualityAssessment(currentReading);

        // 更新统计数据
        updateStatistics(currentReading);

        Serial.println("✅ 数据读取和处理完成");
      } else {
        stats.error_count++;
        Serial.println("⚠️ 接收到数据但验证失败");
      }
    } else {
      stats.error_count++;
      Serial.println("❌ 未接收到有效数据");

      // 检查连接状态
      if (!sensorSerial) {
        Serial.println("🔌 UART连接异常，尝试重新初始化...");
        initUART();
      }
    }

    // 显示连接统计
    float successRate = (stats.total_readings > 0) ?
                       (float)stats.valid_readings / stats.total_readings * 100.0 : 0.0;
    Serial.printf("📊 统计: 总计%u次, 成功%u次, 错误%u次, 成功率%.1f%%\n",
                  stats.total_readings, stats.valid_readings, stats.error_count, successRate);
  }

  // 定时打印统计信息
  if (currentTime - lastStatsTime >= STATS_INTERVAL) {
    lastStatsTime = currentTime;
    printStatistics();
  }

  // 处理串口命令
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "stats") {
      printStatistics();
    } else if (command == "debug") {
      debugUARTConnection();
    } else if (command == "test") {
      performSensorTest();
    } else if (command == "reset") {
      initStats();
      Serial.println("📊 统计数据已重置");
    } else if (command == "help") {
      Serial.println("可用命令:");
      Serial.println("  stats  - 显示统计信息");
      Serial.println("  debug  - 调试UART连接");
      Serial.println("  test   - 执行传感器测试");
      Serial.println("  reset  - 重置统计数据");
      Serial.println("  help   - 显示帮助信息");
    } else if (command.length() > 0) {
      Serial.println("❓ 未知命令，输入 'help' 查看可用命令");
    }
  }

  delay(100); // 短暂延时
}

// ===== 函数实现 =====

void initUART() {
  Serial.println("🔧 正在初始化UART通信...");

  // 配置UART参数: 波特率, 数据位, 停止位, 校验位
  sensorSerial.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  // 等待串口稳定
  delay(100);

  if (sensorSerial) {
    Serial.println("✅ UART初始化成功");
    Serial.printf("   配置: %d bps, 8N1\n", UART_BAUD_RATE);
    Serial.printf("   RX: GPIO%d, TX: GPIO%d\n", UART_RX_PIN, UART_TX_PIN);

    // 清空接收缓冲区
    int cleared = 0;
    while (sensorSerial.available()) {
      sensorSerial.read();
      cleared++;
    }
    if (cleared > 0) {
      Serial.printf("   清空了%d字节旧数据\n", cleared);
    }
  } else {
    Serial.println("❌ UART初始化失败");
  }
}

void initStats() {
  Serial.println("📊 初始化统计数据...");

  // TVOC统计
  stats.tvoc_sum = 0;
  stats.tvoc_min = 99999;
  stats.tvoc_max = 0;

  // 甲醛统计
  stats.hcho_sum = 0;
  stats.hcho_min = 99999;
  stats.hcho_max = 0;

  // CO₂统计
  stats.co2_sum = 0;
  stats.co2_min = 99999;
  stats.co2_max = 0;

  // 计数器
  stats.valid_readings = 0;
  stats.total_readings = 0;
  stats.error_count = 0;

  // 初始化最后读数
  lastReading.valid = false;
  lastReading.timestamp = 0;

  Serial.println("✅ 统计数据初始化完成");
}

bool readVOCSensor(VOCData &data) {
  uint8_t buffer[64];
  int bytesRead = 0;
  unsigned long startTime = millis();

  // 清空接收缓冲区中的旧数据
  while (sensorSerial.available()) {
    sensorSerial.read();
    delay(1);
  }

  // 等待数据到达，超时2秒
  while (bytesRead < sizeof(buffer) && (millis() - startTime) < 2000) {
    if (sensorSerial.available()) {
      buffer[bytesRead] = sensorSerial.read();
      bytesRead++;

      // 如果连续没有新数据超过100ms，认为一帧数据接收完成
      unsigned long lastByteTime = millis();
      while (!sensorSerial.available() && (millis() - lastByteTime) < 100) {
        delay(5);
      }
      if (!sensorSerial.available()) {
        break;
      }
    }
    delay(10);
  }

  if (bytesRead > 0) {
    // 调试：显示接收到的原始数据
    Serial.printf("📡 接收到 %d 字节: ", bytesRead);
    for (int i = 0; i < bytesRead; i++) {
      Serial.printf("%02X ", buffer[i]);
    }
    Serial.println();

    // 解析接收到的数据
    parseVOCData(buffer, bytesRead, data);
    data.timestamp = millis();
    return true;
  } else {
    Serial.println("⏰ 超时，未接收到数据");
  }

  return false;
}

void parseVOCData(uint8_t* buffer, int length, VOCData &data) {
  // 初始化数据结构
  data.tvoc_mgm3 = 0.0;
  data.ch2o_mgm3 = 0.0;
  data.co2_mgm3 = 0.0;
  data.valid = false;

  Serial.printf("🔍 解析数据: 长度=%d ", length);

  // 检查数据帧长度 (根据文档应该是9字节)
  if (length < 9) {
    Serial.printf("数据长度不足（需要9字节，实际%d字节）\n", length);
    return;
  }

  // 寻找正确的数据帧起始位置 (0x2C模块地址开头)
  int frameStart = -1;
  for (int i = 0; i <= length - 9; i++) {
    if (buffer[i] == 0x2C && (i + 1 < length) && buffer[i + 1] == 0xE4) {
      frameStart = i;
      Serial.printf("找到帧头0x2C 0xE4在位置%d ", i);
      break;
    }
  }

  if (frameStart == -1) {
    Serial.println("未找到有效的数据帧头 (0x2C 0xE4)");
    return;
  }

  // 检查是否有足够的字节
  if (frameStart + 9 > length) {
    Serial.printf("数据帧不完整，需要%d字节，实际只有%d字节\n", frameStart + 9, length);
    return;
  }

  // 提取9字节数据帧
  uint8_t frame[9];
  for (int i = 0; i < 9; i++) {
    frame[i] = buffer[frameStart + i];
  }

  // 显示数据帧
  Serial.printf("数据帧: ");
  for (int i = 0; i < 9; i++) {
    Serial.printf("%02X ", frame[i]);
  }
  Serial.println();

  // 计算校验和 (B1+B2+...+B8的低8位)
  uint8_t checksum = 0;
  for (int i = 0; i < 8; i++) {
    checksum += frame[i];
  }
  checksum = checksum & 0xFF; // 取低8位

  // 验证校验和
  if (frame[8] != checksum) {
    Serial.printf("⚠️ 校验和不匹配，但继续解析数据 (接收: 0x%02X, 计算: 0x%02X)\n", frame[8], checksum);
  }

  // 按照文档协议解析数据
  // TVOC浓度 (mg/m³): (B3*256 + B4) × 0.001
  uint16_t tvoc_raw = frame[2] * 256 + frame[3];
  data.tvoc_mgm3 = (float)tvoc_raw * 0.001;

  // 甲醛浓度 (mg/m³): (B5*256 + B6) × 0.001
  uint16_t ch2o_raw = frame[4] * 256 + frame[5];
  data.ch2o_mgm3 = (float)ch2o_raw * 0.001;

  // CO₂浓度 (mg/m³): (B7*256 + B8) × 0.001
  uint16_t co2_raw = frame[6] * 256 + frame[7];
  data.co2_mgm3 = (float)co2_raw * 0.001;

  data.valid = true;

  Serial.printf("✅ 解析成功:\n");
  Serial.printf("  TVOC: %.3f mg/m³\n", data.tvoc_mgm3);
  Serial.printf("  甲醛: %.3f mg/m³\n", data.ch2o_mgm3);
  Serial.printf("  CO₂: %.3f mg/m³\n", data.co2_mgm3);
}

bool validateVOCData(const VOCData &data) {
  // 检查数据有效性标志
  if (!data.valid) {
    Serial.println("❌ 数据无效标志");
    return false;
  }

  // 检查TVOC范围 (0-10 mg/m³)
  if (data.tvoc_mgm3 < 0.0 || data.tvoc_mgm3 > 10.0) {
    Serial.printf("❌ TVOC数值超出范围: %.3f mg/m³\n", data.tvoc_mgm3);
    return false;
  }

  // 检查甲醛范围 (0-2 mg/m³)
  if (data.ch2o_mgm3 < 0.0 || data.ch2o_mgm3 > 2.0) {
    Serial.printf("❌ 甲醛数值超出范围: %.3f mg/m³\n", data.ch2o_mgm3);
    return false;
  }

  // 检查CO₂范围 (0-10 mg/m³)
  if (data.co2_mgm3 < 0.0 || data.co2_mgm3 > 10.0) {
    Serial.printf("❌ CO₂数值超出范围: %.3f mg/m³\n", data.co2_mgm3);
    return false;
  }

  return true;
}

void printVOCReading(const VOCData &data) {
  Serial.println("📊 === VOC-CO2-HCHO传感器读数 ===");
  Serial.printf("⏰ 时间戳: %lu ms\n", data.timestamp);
  Serial.printf("🌿 TVOC浓度: %.3f mg/m³\n", data.tvoc_mgm3);
  Serial.printf("🏠 甲醛(CH₂O): %.3f mg/m³\n", data.ch2o_mgm3);
  Serial.printf("💨 CO₂浓度: %.3f mg/m³\n", data.co2_mgm3);
  Serial.println("===============================");
}

void updateStatistics(const VOCData &data) {
  // 更新TVOC统计
  stats.tvoc_sum += data.tvoc_mgm3;
  if (data.tvoc_mgm3 < stats.tvoc_min) stats.tvoc_min = data.tvoc_mgm3;
  if (data.tvoc_mgm3 > stats.tvoc_max) stats.tvoc_max = data.tvoc_mgm3;

  // 更新甲醛统计
  stats.hcho_sum += data.ch2o_mgm3;
  if (data.ch2o_mgm3 < stats.hcho_min) stats.hcho_min = data.ch2o_mgm3;
  if (data.ch2o_mgm3 > stats.hcho_max) stats.hcho_max = data.ch2o_mgm3;

  // 更新CO₂统计
  stats.co2_sum += data.co2_mgm3;
  if (data.co2_mgm3 < stats.co2_min) stats.co2_min = data.co2_mgm3;
  if (data.co2_mgm3 > stats.co2_max) stats.co2_max = data.co2_mgm3;
}

void printStatistics() {
  if (stats.valid_readings == 0) {
    Serial.println("📊 暂无有效统计数据");
    return;
  }

  Serial.println();
  Serial.println("📊 ========== 统计报告 ==========");
  Serial.printf("📈 数据概况: 总计%u次读取, 成功%u次, 错误%u次\n",
                stats.total_readings, stats.valid_readings, stats.error_count);

  float successRate = (float)stats.valid_readings / stats.total_readings * 100.0;
  Serial.printf("✅ 成功率: %.1f%%\n", successRate);

  Serial.println();
  Serial.println("🌿 TVOC浓度统计 (mg/m³):");
  Serial.printf("   平均值: %.3f\n", stats.tvoc_sum / stats.valid_readings);
  Serial.printf("   最小值: %.3f\n", stats.tvoc_min);
  Serial.printf("   最大值: %.3f\n", stats.tvoc_max);

  Serial.println("🏠 甲醛(CH₂O)统计 (mg/m³):");
  Serial.printf("   平均值: %.3f\n", stats.hcho_sum / stats.valid_readings);
  Serial.printf("   最小值: %.3f\n", stats.hcho_min);
  Serial.printf("   最大值: %.3f\n", stats.hcho_max);

  Serial.println("💨 CO₂浓度统计 (mg/m³):");
  Serial.printf("   平均值: %.3f\n", stats.co2_sum / stats.valid_readings);
  Serial.printf("   最小值: %.3f\n", stats.co2_min);
  Serial.printf("   最大值: %.3f\n", stats.co2_max);

  Serial.println("================================");
}

void printDataQualityAssessment(const VOCData &data) {
  Serial.println("🔍 === 数据质量评估 ===");

  // TVOC浓度评估
  String tvocLevel = getTVOCQualityLevel(data.tvoc_mgm3);
  Serial.printf("🌿 TVOC浓度: %s (%.3f mg/m³)\n", tvocLevel.c_str(), data.tvoc_mgm3);

  // 甲醛安全性评估
  String hchoLevel = getHCHOSafetyLevel(data.ch2o_mgm3);
  Serial.printf("🏠 甲醛安全性: %s (%.3f mg/m³)\n", hchoLevel.c_str(), data.ch2o_mgm3);

  // CO₂浓度评估
  String co2Level = getCO2ComfortLevel(data.co2_mgm3);
  Serial.printf("💨 CO₂浓度: %s (%.3f mg/m³)\n", co2Level.c_str(), data.co2_mgm3);

  Serial.println("=====================");
}

String getTVOCQualityLevel(float tvoc) {
  if (tvoc <= TVOC_EXCELLENT) return "优秀 ✅";
  else if (tvoc <= TVOC_GOOD) return "良好 🟢";
  else if (tvoc <= TVOC_MODERATE) return "一般 🟡";
  else if (tvoc <= TVOC_POOR) return "较差 🟠";
  else return "很差 🔴";
}

String getHCHOSafetyLevel(float hcho) {
  if (hcho <= HCHO_SAFE) return "安全 ✅";
  else if (hcho <= HCHO_ACCEPTABLE) return "可接受 🟢";
  else if (hcho <= HCHO_CONCERNING) return "需关注 🟡";
  else if (hcho <= HCHO_DANGEROUS) return "危险 🔴";
  else return "极危险 ⚠️";
}

String getCO2ComfortLevel(float co2) {
  if (co2 <= CO2_FRESH) return "新鲜空气 ✅";
  else if (co2 <= CO2_ACCEPTABLE) return "可接受 🟢";
  else if (co2 <= CO2_DROWSY) return "令人困倦 🟡";
  else if (co2 <= CO2_STUFFY) return "闷热 🟠";
  else return "非常闷热 🔴";
}

void debugUARTConnection() {
  Serial.println("🔧 === UART连接调试 ===");

  // 检查串口状态
  Serial.printf("串口状态: %s\n", sensorSerial ? "正常" : "异常");
  Serial.printf("波特率: %d\n", UART_BAUD_RATE);
  Serial.printf("RX引脚: GPIO%d\n", UART_RX_PIN);
  Serial.printf("TX引脚: GPIO%d\n", UART_TX_PIN);

  // 检查缓冲区
  int available = sensorSerial.available();
  Serial.printf("缓冲区可用字节: %d\n", available);

  if (available > 0) {
    Serial.print("缓冲区内容: ");
    while (sensorSerial.available()) {
      Serial.printf("%02X ", sensorSerial.read());
    }
    Serial.println();
  }

  // 连接统计
  float successRate = (stats.total_readings > 0) ?
                     (float)stats.valid_readings / stats.total_readings * 100.0 : 0.0;
  Serial.printf("连接统计: 总计%u次, 成功%u次, 成功率%.1f%%\n",
                stats.total_readings, stats.valid_readings, successRate);

  Serial.println("====================");
}

void performSensorTest() {
  Serial.println("🧪 === 传感器连接测试 ===");

  // 重新初始化UART
  Serial.println("1. 重新初始化UART...");
  initUART();

  // 尝试读取数据
  Serial.println("2. 尝试读取传感器数据...");
  VOCData testData;

  for (int i = 0; i < 3; i++) {
    Serial.printf("   测试 %d/3: ", i + 1);

    if (readVOCSensor(testData)) {
      if (testData.valid && validateVOCData(testData)) {
        Serial.println("✅ 成功");
        printVOCReading(testData);
        break;
      } else {
        Serial.println("❌ 数据无效");
      }
    } else {
      Serial.println("❌ 无数据");
    }

    delay(1000);
  }

  Serial.println("========================");
}