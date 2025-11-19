# ESP32-S3 Voice Assistant Wiring

## Voice Assistant (Full Setup)

```
ESP32-S3 Pin Connections:

INMP441 Microphone:
  VDD  → 3.3V
  GND  → GND
  SD   → GPIO8  (I2S Data)
  WS   → GPIO6  (I2S LRC)
  SCK  → GPIO7  (I2S BCLK)
  L/R  → GND    (Left Channel)

MAX98357A Speaker:
  VIN  → 5V
  GND  → GND
  DIN  → GPIO18 (I2S Data)
  BCLK → GPIO17 (I2S BCLK)
  LRC  → GPIO16 (I2S LRC)
  GAIN → GND    (9dB gain)
  [+][-] → Speaker 4-8Ω

WS2812 Status LED:
  VCC  → 5V (or 3.3V)
  GND  → GND
  DIN  → GPIO48

Push Button (Optional):
  One end → GPIO9 (pull-up enabled)
  Other   → GND
```

## Microphone Only (Basic Setup)

```
ESP32-S3 Pin Connections:

INMP441 Microphone:
  VDD  → 3.3V
  GND  → GND
  SD   → GPIO8  (I2S Data)
  WS   → GPIO6  (I2S LRC)
  SCK  → GPIO7  (I2S BCLK)
  L/R  → GND    (Left Channel)
```

## Quick Reference

| Component | Pins Used | Power | Notes |
|-----------|-----------|-------|-------|
| INMP441 Mic | GPIO6,7,8 | 3.3V | L/R→GND for left channel |
| MAX98357A Speaker | GPIO16,17,18 | 5V | Need 1A+ for loud audio |
| WS2812 LED | GPIO48 | 5V | Single LED or strip |
| Push Button | GPIO9 | - | Internal pull-up |

## INMP441 Channel Selection

```
L/R Pin → GND    = Left channel  (config: channel: left)
L/R Pin → 3.3V   = Right channel (config: channel: right)
```

## MAX98357A Gain Settings

```
GAIN Pin → GND = 9dB  (recommended)
GAIN Pin → VDD = 15dB (if too quiet)
```

## LED Status Indicators

- 🟢 Fast pulse = Listening
- 🟢 Solid green = Processing speech
- 🔵 Solid blue = Speaking (TTS)
- 🔴 Red flash = Error

## Power Requirements

| Component | Voltage | Current |
|-----------|---------|---------|
| ESP32-S3 | 3.3V | ~250mA |
| INMP441 | 3.3V | ~2mA |
| MAX98357A (idle) | 5V | ~3mA |
| MAX98357A (loud) | 5V | ~600mA |
| WS2812 (per LED) | 5V | ~60mA |

**Recommended**: 5V 1.5A power supply (USB is usually sufficient)

## Notes

- ⚠️ INMP441 L/R pin must be connected (GND or VDD)
- ⚠️ Use 4-8Ω speaker (not headphones directly)
- ⚠️ MAX98357A needs sufficient current for loud audio
- ✅ WS2812 requires only 3 wires (simpler than RGB LED)
- ✅ If using LED strip, adjust `num_leds` in config
