# LD2450 Radar Sensor ESPHome Configuration

This configuration converts the Arduino C++ code for the LD2450 radar sensor to ESPHome YAML format.

## Hardware Setup

- **ESP32-C3 DevKit**: Main microcontroller
- **LD2450 Radar Sensor**: 24GHz mmWave radar for human presence detection
- **Connections**:
  - LD2450 TX → ESP32-C3 GPIO1 (RX)
  - LD2450 RX → ESP32-C3 GPIO0 (TX)
  - LD2450 VCC → 3.3V
  - LD2450 GND → GND

## Features

### Sensors

- **Target 1 Position**: X, Y coordinates for the first detected target (in mm)
- **Target 1 Speed**: Movement speed for the first target (in cm/s)
- **Target 1 Distance**: Calculated distance from sensor (in meters)
- **Target 1 Angle**: Angle relative to sensor (in degrees)
- **Target Count**: Number of detected targets (currently supports 1 target)

### Binary Sensors

- **Detection Active**: Shows if detection is currently enabled
- **Target 1 Detected**: Detection status for the first target

### Controls

- **Enable Detection Switch**: Turn detection on/off
- **Radar Status**: Text sensor showing current status

## Configuration Files

1. **radar-esp32c3.yaml**: Main ESPHome configuration with inline LD2450 protocol implementation
2. **secrets.yaml**: WiFi and other sensitive configuration
3. **LD2450_README.md**: This documentation file

## Protocol Details

The LD2450 uses a custom serial protocol at 256000 baud:
- **Frame Header**: AA FF 03 00
- **Data Length**: 24 bytes (3 targets × 8 bytes each)
- **Frame Tail**: 55 CC
- **Total Frame**: 30 bytes (60 hex characters)

Each target contains:
- X coordinate (2 bytes, signed, little-endian)
- Y coordinate (2 bytes, signed, little-endian)
- Speed (2 bytes, signed, little-endian)
- Resolution (2 bytes, unsigned, little-endian)

## Usage

1. Update `secrets.yaml` with your WiFi credentials
2. Flash the configuration to your ESP32-C3:
   ```bash
   esphome run radar-esp32c3.yaml
   ```
3. The device will appear in Home Assistant with all sensors
4. Use the "Enable Detection" switch to control radar operation

## Home Assistant Integration

All sensors will automatically appear in Home Assistant:

- `sensor.radar_esp32c3_target_1_x_position`
- `sensor.radar_esp32c3_target_1_y_position`
- `sensor.radar_esp32c3_target_1_distance`
- `sensor.radar_esp32c3_target_1_angle`
- `sensor.radar_esp32c3_target_1_speed`
- `sensor.radar_esp32c3_target_count`
- `binary_sensor.radar_esp32c3_target_1_detected`
- `binary_sensor.radar_esp32c3_detection_active`
- `switch.radar_esp32c3_enable_detection`
- `text_sensor.radar_esp32c3_radar_status`

## Troubleshooting

1. **No data received**: Check wiring and baud rate (256000)
2. **Invalid frames**: Check for electromagnetic interference
3. **Compilation errors**: Ensure all files are in the same directory
4. **WiFi issues**: Check secrets.yaml configuration

## Differences from Original Arduino Code

- Removed web server functionality (ESPHome provides this)
- Removed LittleFS data storage (Home Assistant handles history)
- Simplified to focus on sensor data collection
- Uses ESPHome global variables and interval components instead of custom C++ classes
- Currently supports 1 target instead of 3 (can be extended)
- Maintained core LD2450 protocol parsing logic
- Uses ESPHome's built-in UART component

## Advanced Configuration

You can modify the update intervals, add filters, or create automations in Home Assistant based on the sensor data. The current implementation focuses on the first detected target but can be extended to support multiple targets by adding more global variables and parsing logic.
