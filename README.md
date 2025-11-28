# Stellar Vision V1

![License: PolyForm Noncommercial 1.0.0](https://img.shields.io/badge/License-PolyForm%20Noncommercial%201.0.0-blue)
![Firmware Version](https://img.shields.io/badge/Firmware-v2.1-green)
![Project Status](https://img.shields.io/badge/Status-Active-brightgreen)

**An Assistive Braille Input Device with AI Integration**

Stellar Vision V1 is an advanced assistive technology device designed for visually impaired users, featuring a 14-key Braille input system, AI-powered functionalities, and comprehensive connectivity options.

## ✨ Key Features

- **🟠 Braille Input System**: 14-key input with 6-dot Braille pattern recognition
- **🤖 AI Integration**: Query Gemini AI directly from the device
- **🔊 Multi-Modal Feedback**: Audio feedback, text-to-speech, and vibration alerts
- **📁 File Management**: SD card storage with wireless upload capabilities
- **🔗 Connectivity**: HID keyboard output, BLE notifications, WiFi for OTA updates
- **🆘 Safety Features**: SOS emergency alert system via SMS
- **🎯 Accessibility**: One-hand gesture mode via MPU6050 accelerometer

## 🛠 Technical Specifications

| Component | Specification |
|-----------|---------------|
| **MCU** | ESP32C6 (dual-core, 160MHz) |
| **GPIO Expander** | PCF8575 (I2C address 0x20) |
| **HID Interface** | DigiSpark ATtiny85 (I2C address 0x23) |
| **IMU** | MPU6050 (I2C address 0x68) |
| **Audio** | MAX98357A I2S amplifier |
| **Storage** | SD card (SPI, FAT32 format) |
| **Connectivity** | BLE (ChronosESP32), WiFi |

## 📦 Repository Structure

```
Stellar_Vision_V1/
├── Firmware/
│   ├── releases/
│   │   └── firmware_v2.1/      # Latest firmware release
│   └── latest.json            # Firmware update information
├── Manual Sheet/
│   ├── Stellar Vision V1 - MANUAL.pdf
│   └── Stellar_Vision_V1-MANUAL.md
├── LICENCE.txt               # PolyForm Noncommercial 1.0.0
└── README.md
```

## 🚀 Getting Started

### Prerequisites
- SD card formatted as FAT32
- Arduino IDE or compatible ESP32 development environment
- Required libraries: PCF8575, ArduinoJson, WiFi, HTTPClient

### Installation
1. Clone this repository
2. Insert SD card with required folder structure
3. Upload firmware to ESP32C6
4. Configure API keys for Gemini AI, VoiceRSS, OCR.space, and Twilio

### Basic Usage
- **Power On**: Device boots into NORMAL MODE
- **Typing**: Short press SELECT to enter PERKINS MODE for Braille input
- **Mode Selection**: Long press SELECT to access MODE OPTIONS
- **Emergency**: Hold either SPACE button for 1.5s to trigger SOS alert

## 📚 Documentation

- **[Full User Manual](Manual%20Sheet/Stellar_Vision_V1-MANUAL.md)** - Comprehensive usage instructions
- **[Firmware Releases](Firmware/releases/firmware_v2.1)** - Latest firmware and updates
- **[Manual PDF](Manual%20Sheet/Stellar%20Vision%20V1%20-%20MANUAL.pdf)** - Printable manual version

## 🔧 Operating Modes

| Mode | Purpose | Key Feature |
|------|---------|-------------|
| **PERKINS MODE** | Braille typing | Real-time HID keyboard output |
| **GEMINI AI** | AI queries | Google Gemini AI integration |
| **NOTE-MAKER** | Note taking | SD card storage with auto-correction |
| **B-DRIVE** | OCR processing | Text extraction from images/PDFs |
| **SD UPLOAD** | File management | Wireless web interface for file transfer |
| **AUDIO PLAYER** | Media playback | WAV file support with navigation controls |

## 🆘 Emergency Features

The device includes a comprehensive SOS system:
- **Activation**: Hold either SPACE button for 1.5 seconds
- **Notification**: Sends SMS via Twilio with device status and uptime
- **Safety**: 30-second cooldown between alerts to prevent accidental triggers

## 🔄 Firmware Updates

- **OTA Updates**: Available via SYSTEM UPDATE mode
- **Manual Updates**: Flash through Arduino IDE
- **Version Check**: Automatic checking against `Firmware/latest.json`

## 📄 License

This project is licensed under the **PolyForm Noncommercial License 1.0.0** - see the [LICENCE.txt](LICENCE.txt) file for details.

**Required Notice**: Copyright © 2025 MAATHES THILAK K (https://github.com/MAATHES-THILAK-K)

## 👨‍💻 Author

**Maathhes Thilak K**  
Bachelor of Engineering – ECE  
Madras Institute of Technology (MIT), Anna University, Chennai

**Development Philosophy**:  
*"Vibe Coding — Leveraging AI as a collaborative tool to accelerate innovation and create meaningful accessibility solutions."*

## 🤝 Contributing

While this is a personal project, feedback and suggestions are welcome. Please ensure compliance with the noncommercial license terms.

---

**Stellar Vision V1 – Making Technology Accessible**  
*Firmware Version: 2.1 | Last Updated: November 2025*
