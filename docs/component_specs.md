# 📊 Component Specifications

## ESP32-C3 Microcontroller

| Parameter | Specification |
|-----------|---------------|
| Core | 32-bit RISC-V single-core processor |
| Frequency | 160 MHz |
| SRAM | 400 KB |
| Flash | 4 MB |
| WiFi | 802.11 b/g/n (2.4 GHz) |
| Bluetooth | Bluetooth 5 LE |
| GPIO | 22 programmable pins |
| ADC | 6 channels, 12-bit |
| PWM | Multiple channels |
| Operating Voltage | 3.0V - 3.6V |

## GPS Module (NEO-6M)

| Parameter | Specification |
|-----------|---------------|
| Chipset | u-blox NEO-6M |
| Frequency | L1, 1575.42 MHz |
| Channels | 50 |
| Sensitivity | -161 dBm |
| Accuracy | 2.5m CEP |
| Update Rate | 1 Hz (up to 5 Hz) |
| Interface | UART, 9600 baud default |
| Operating Voltage | 3.3V |
| Current | 45 mA |

## OLED Display (SSD1306)

| Parameter | Specification |
|-----------|---------------|
| Size | 0.96 inch |
| Resolution | 128 x 64 pixels |
| Interface | I2C (0x3C) |
| Color | Monochrome (white) |
| Viewing Angle | > 160° |
| Operating Voltage | 3.3V - 5V |
| Current | ~20mA active |

## Temperature Sensor (DS18B20)

| Parameter | Specification |
|-----------|---------------|
| Range | -55°C to +125°C |
| Accuracy | ±0.5°C (-10°C to +85°C) |
| Resolution | 9 to 12 bits selectable |
| Interface | OneWire |
| Power | 3.0V to 5.5V |
| Conversion Time | 750ms (max at 12-bit) |

## Voltage Sensor (INA226)

| Parameter | Specification |
|-----------|---------------|
| Bus Voltage | 0-36V |
| Shunt Voltage | ±81.92mV |
| Interface | I2C (0x40-0x4F) |
| Accuracy | 0.1% (gain error) |
| Power | 2.7V - 5.5V |
| Current | 330μA (typical) |

## RTC Module (DS1307)

| Parameter | Specification |
|-----------|---------------|
| Interface | I2C (0x68) |
| Accuracy | ±2ppm from 0°C to +40°C |
| Backup | Battery (CR2032) |
| Operating Voltage | 3.3V - 5.5V |
| Time Keeping | Years, months, dates, hours, minutes, seconds |