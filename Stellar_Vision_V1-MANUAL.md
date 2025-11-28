# **Sᴛᴇʟʟᴀʀ Vɪsɪᴏɴ ᴠ1 — USER MANUAL**
### *Assistive Braille Input Device with AI Integration*

***

## Table of Contents
1. [Device Overview](#device-overview)
2. [Button Layout](#button-layout)
3. [Getting Started](#getting-started)
4. [Operating Modes](#operating-modes)
5. [Navigation Guide](#navigation-guide)
6. [Advanced Features](#advanced-features)
7. [Quick Reference Card](#quick-reference-card)
8. [Technical Specifications](#technical-specifications)
9. [Support and Resources](#support-and-resources)
10. [About the Author](#about-the-author)

***

## Device Overview

**Sᴛᴇʟʟᴀʀ Vɪsɪᴏɴ** is an assistive technology device designed for visually impaired users, featuring:

- **14-key Braille input system** via PCF8575 GPIO expander  
- **6-dot Braille pattern recognition** with full character set  
- **HID keyboard output** via DigiSpark (USB keyboard emulation)  
- **AI integration** (Gemini AI for queries)  
- **Notification system** via BLE (ChronosESP32)  
- **Audio feedback system** with voice confirmations and mode announcements  
- **SD card storage** for notes and files  
- **OCR capabilities** (*B-DRIVE mode* for text extraction from images/PDFs)  
- **Text-to-speech** for reading files aloud  
- **One-hand gesture mode** via MPU6050 accelerometer  
- **SOS Emergency Alert** via SMS (Twilio integration)  
- **OTA firmware updates** via WiFi  
- **SD Upload Mode** for wireless file transfer  
- **Audio Player** for WAV file playback  
- **Alphabet audio feedback** for typed characters  
- **Text auto-correction** system  
- **Deep sleep mode** for power saving  

***

## Button Layout

### Input Keys (14 Total)

| Pin | Function | Description |
|-----|-----------|-------------|
| 0–5 | Braille Dots 1–6 | Standard 6-dot Braille input |
| 6 | Ctrl Modifier | Toggle Ctrl key (for combos) |
| 7 | Backspace | Delete last character |
| 8 | Space (Left) | Space bar / Modifier / **SOS trigger (long press)** |
| 9 | Space (Right) | Space bar / Modifier / **SOS trigger (long press)** |
| 10 | Previous | Go back / Long press for gesture mode |
| 14 | Up | Navigate up in menus / **Long press toggles alphabet audio** |
| 12 | Select | Confirm / Enter mode |
| 13 | Down | Navigate down in menus |

***

## Getting Started

### Initial Boot
1. Power on the device  
2. Wait for initialization (vibration feedback)  
3. Device enters **NORMAL MODE** by default  
4. Serial Print: *"Stellar Vision v1"*

### First-Time Setup
- **SD Card**: Insert before powering on for file storage  
  - Create `/AudioFiles/` folder for audio player  
  - Create `/Alphabets/` folder for letter pronunciation (optional)
- **BLE Connection**: Pair with mobile device for notifications  
- **HID Mode**: Connect DigiSpark for keyboard functionality  
- **Twilio SMS**: Configure credentials for SOS emergency alerts  

***

## Operating Modes

### 1. NORMAL MODE
**Entry**: Device boots into this mode  
**Purpose**: Idle state, ready to enter other modes  

**Actions:**
- **Select (short press)** → Enter *PERKINS MODE*  
- **Select (long press)** → Open *MODE OPTIONS*

**Power Management:**
- Device enters **deep sleep** after 30 seconds of inactivity
- Power cycle to wake from deep sleep

***

### 2. PERKINS MODE
**Entry:** Short press SELECT from NORMAL MODE  
**Purpose:** Braille typing with real-time HID keyboard output  

**Features:**
- Type Braille patterns (dots 1–6)  
- Characters appear on connected computer via HID  
- **Space** → Add space character  
- **Backspace** → Delete last character  
- **Ctrl (toggle)** → Hold Ctrl modifier  
- **Alphabet Audio** → Announces typed letters (when enabled)

#### Special Indicators
- **Pattern 60** (dots 3,4,5,6): Number mode  
- **Pattern 32** (dot 6): Capital mode  

#### Key Combinations
| Combination | Function |
|--------------|-----------|
| Space + Backspace (hold) | Enter key |
| Space + Ctrl (hold) | Shift + Enter (newline) |
| **Ctrl + Space (hold)** | **Apply text correction** |
| **Space LEFT/RIGHT (1.5s hold)** | **Send SOS emergency SMS** |

**Exit:** Press PREVIOUS to return to MODE OPTIONS

***

### 3. MODE OPTIONS
**Entry:** Long press SELECT from any mode  
**Purpose:** Navigate between device features  

**Available Modes:**
1. NOTIFY  
2. GEMINI AI  
3. NOTE-MAKER  
4. HID SHORTCUT  
5. B-DRIVE  
6. SD CARD  
7. SD UPLOAD
8. SYSTEM UPDATE
9. AUDIO PLAYER

**Navigation:**
- **Up/Down:** Scroll through options  
- **Select:** Choose highlighted option  
- **Previous:** Return to PERKINS MODE  

***

### 4. NOTE-MAKER MODE
**Entry:** Select "NOTE-MAKER" from MODE OPTIONS  
**Purpose:** Create and save text notes to SD card  

#### Workflow
1. **Naming Phase**
   - Type desired filename (max 12 characters)  
   - Double-tap SELECT to confirm name  
   - Double-tap SELECT again for auto name (NOTE1.txt, NOTE2.txt…)

2. **Writing Phase**
   - Type note using Braille input  
   - BACKSPACE → Correct errors  
   - SPACE → Insert space  
   - **Ctrl + Space → Apply text correction**

3. **Saving**
   - **Ctrl + Backspace (hold)** → Save and exit  
   - Audio feedback confirms save  

**Exit:** PREVIOUS before saving → Discard note  

***

### 5. SD CARD MODE
**Entry:** Select "SD CARD" from MODE OPTIONS  
**Purpose:** Browse and read files on SD card  

**Navigation:**
- **Up/Down:** Navigate file list  
- **Select:** Open file (printed to serial)  
- **Ctrl + Backspace (hold):  Text-to-speech (TTS) of current file** 

**File Compatibility:**  
`.txt` (UTF-8 text)  

**Power Note:** Deep sleep is disabled in this mode

**Exit:** Press PREVIOUS  

***

### 6. GEMINI AI MODE
**Entry:** Select "GEMINI AI" from MODE OPTIONS  
**Purpose:** Query Google Gemini AI and save responses  

#### Workflow
1. **Query Phase:** Type your question in Braille  
2. **Send Query:** Ctrl + Backspace (hold)  
3. **Save Response:** Type filename → Double-tap SELECT → Confirm

**Requirements:**  
- WiFi connection (auto-connects on mode entry)  
- Internet access  
- Pre-configured Gemini API key  

**Exit:** PREVIOUS  

***

### 7. B-DRIVE MODE
**Entry:** Select "B-DRIVE" from MODE OPTIONS  
**Purpose:** Extract text from images/PDFs via OCR  

#### Setup
1. Announces local web server IP (port 80)  
2. Connect device to same WiFi network  
3. Access via browser at `http://[IP_ADDRESS]`  

#### Supported Formats
- Images: JPG, PNG  
- PDFs: Text extraction (client-side)  

**Workflow:**
1. Upload file via web interface  
2. OCR processes text (OCR.space API)  
3. Device vibrates when complete  
4. Save: Type filename → Double-tap SELECT → Ctrl + Backspace

**Requirements:**
- WiFi + OCR.space API key  
- SD card  

**Power Note:** Deep sleep is disabled in this mode

**Exit:** PREVIOUS (stops web server)  

***

### 8. HID SHORTCUT MODE
**Entry:** Select "HID SHORTCUT" from MODE OPTIONS  
**Purpose:** Execute predefined keyboard shortcuts  

| Shortcut | Action |
|-----------|--------|
| WIFI | Open WiFi settings |
| CHROME | Launch Chrome |
| GMAIL | Open Gmail |
| CHATGPT | Open ChatGPT |
| PY COMPILER | Open Python IDE |
| LIBRARY | Open Library app |

**Navigation:**
- **Up/Down:** Browse shortcuts  
- **Select:** Execute highlighted shortcut  

**Requirements:** DigiSpark HID connected  
**Exit:** PREVIOUS  

***

### 9. NOTIFY MODE
**Entry:** Select "NOTIFY" from MODE OPTIONS  
**Purpose:** Receive and log smartphone notifications  

**Features:**
- BLE connection via ChronosESP32  
- Logs notifications to `/NOTIFY.txt`  
- Ring buffer (8 messages)  

**Log Format:**
```
App: [app_name] [timestamp]
Msg: [title] [message]
---------------------
```

**Toggle:**
- Select "NOTIFY" → ON (vibration feedback)  
- Select again → OFF  

**Note:** Old `NOTIFY.txt` is cleared on each boot

***

### 10. SD UPLOAD MODE
**Entry:** Select "SD UPLOAD" from MODE OPTIONS  
**Purpose:** Upload files and folders wirelessly to SD card  

#### Features
- Web-based file upload interface (port 8080)  
- **Folder structure support** with drag-and-drop  
- Browse, download, and delete files  
- Directory tree visualization  

#### Setup
1. Device announces IP address on port 8080  
2. Access `http://[IP_ADDRESS]:8080` in browser  
3. Upload individual files or entire folders  

**File Management:**
- View all files with sizes  
- Download files to computer  
- Delete files remotely  
- **Create nested folder structures**  

**Requirements:**
- WiFi connection  
- SD card  

**Power Note:** Deep sleep is disabled in this mode

**Exit:** PREVIOUS (stops server)  

***

### 11. SYSTEM UPDATE MODE
**Entry:** Select "SYSTEM UPDATE" from MODE OPTIONS  
**Purpose:** Check for and install firmware updates via OTA  

#### Workflow
1. **Check for Updates**
   - Device connects to GitHub repository  
   - Compares current firmware (v2.0) with latest version  

2. **Update Available**
   - Device announces new version  
   - Audio confirmation plays  
   - **SELECT:** Confirm and install update  
   - **PREVIOUS:** Cancel update  

3. **Installation**
   - Progress indicators via vibration  
   - ⚠️ **DO NOT POWER OFF** during update  
   - Device reboots automatically after successful update  

**Timeout:** 15 seconds to confirm update  

**Power Note:** Deep sleep is disabled in this mode

**Exit:** PREVIOUS (cancels update)  

***

### 12. AUDIO PLAYER MODE
**Entry:** Select "AUDIO PLAYER" from MODE OPTIONS  
**Purpose:** Play WAV audio files from SD card  

#### Setup
- Place `.wav` files in `/AudioFiles/` folder on SD card  
- Files are automatically sorted alphabetically  

#### Controls
| Button | Function |
|---------|----------|
| **UP** | Previous track (or stop if playing) |
| **DOWN** | Next track (or stop if playing) |
| **SELECT** | Play/Stop current track |
| **PREVIOUS** | Exit Audio Player Mode |

**Features:**
- Supports WAV format (22kHz 16-bit mono recommended)  
- Audio feedback for track changes  
- File list announced on entry  

**Requirements:**
- SD card with `/AudioFiles/` folder  
- WAV files  

**Power Note:** Deep sleep is disabled in this mode

**Exit:** PREVIOUS (stops playback and returns to MODE OPTIONS)  

***

## Navigation Guide

### Global Button Combinations

| Combination | Function |
|--------------|-----------|
| Ctrl + Backspace (hold) | Save / Send / TTS |
| Space + Backspace (hold) | Enter |
| Space + Ctrl (hold) | Shift + Enter |
| **Ctrl + Space (hold)** | **Apply text correction** |
| **Space LEFT/RIGHT (1.5s hold)** | **SOS Emergency SMS** |
| SELECT (long press) | Open MODE OPTIONS |
| PREVIOUS (long press) | Toggle ONE-HAND MODE |
| **UP (long press 1s)** | **Toggle alphabet audio feedback** |
| **All 6 dots pressed** | **Toggle alphabet audio (alternative)** |

### Mode Transitions

```
NORMAL MODE  
↓ (SELECT short)  
PERKINS MODE  
↓ (SELECT long)  
MODE OPTIONS  
↓ (SELECT on option)  
[Selected Mode]  
↓ (PREVIOUS)  
MODE OPTIONS  
↓ (PREVIOUS)  
PERKINS MODE  
```

***

## Advanced Features

### ONE-HAND GESTURE MODE
**Activation:** Long press PREVIOUS  
**Purpose:** Control device via MPU6050 accelerometer  

| Tilt | Action |
|-------|---------|
| Right | SELECT |
| Left | PREVIOUS |
| Up | UP |
| Down | DOWN |
| Long Up (3s) | Disable Gesture Mode |

***

### SOS Emergency Alert System
**Activation:** Long press either SPACE button for **1.5 seconds**  
**Provider:** Twilio SMS API  

**Features:**
- Sends emergency SMS to pre-configured number  
- Includes device status and uptime  
- **30-second cooldown** between alerts (prevents spam)  
- Audio announcement: `/TACTI_VISION_WAV/SOS.wav`  

**SMS Content:**
```
SOS TRIGGERED FROM STELLAR VISION
STATUS: Emergency
DEVICE: TACTI-WAVE
UPTIME: [hours]h [minutes]m [seconds]s
IMMEDIATE ASSISTANCE REQUIRED
```

**Requirements:**
- WiFi connection (auto-connects if needed)  
- Twilio account credentials configured  

**Cooldown:** Cannot send duplicate alerts within 30 seconds

***

### Text Auto-Correction System
**Activation:** Ctrl + Space (hold 400ms)  
**Purpose:** Correct common typing mistakes  

**Supported Corrections:**
| Typo | Correction |
|------|------------|
| teh | the |
| pleese | please |
| tnanks | thanks |
| heelp | help |
| cal | call |
| fone | phone |
| mesage | message |
| reed | read |
| adio | audio |
| braile | braille |
| scren | screen |
| voise | voice |
| setings | settings |
| batery | battery |
| emergncy | emergency |

**Available In:** PERKINS MODE, NOTE-MAKER MODE, GEMINI AI MODE, B-DRIVE naming, GEMINI naming

**Feedback:** Vibration + serial output showing corrections made

***

### Alphabet Audio Feedback
**Toggle:** Long press UP (1 second) **OR** press all 6 Braille dots  
**Audio Source:** `/Alphabets/A.wav` through `/Alphabets/Z.wav`  

**Features:**
- Announces each letter as you type  
- Works with both lowercase and uppercase  
- Disabled by default to save processing time  
- Available in all typing modes  

**File Format:** WAV files named `A.wav`, `B.wav`, etc.

***

### Audio Feedback System
**Hardware:** MAX98357A I2S amplifier  

**Features:**
- Mode entry notifications  
- Action confirmations  
- Gesture mode announcements  
- **Alphabet pronunciation** (when enabled)  
- **Deep sleep announcement**  
- **SOS alert sound**  

**I2C/I2S Conflict Prevention:**
- HID keyboard temporarily paused during audio  
- I2C bus properly reinitialized after playback  
- DigiSpark reconnection handled automatically  

***

### Text-to-Speech (TTS)
**Trigger:** Ctrl + Backspace (in SD CARD MODE)  
**Provider:** VoiceRSS API  

**Features:**
- Reads text aloud in **300-character chunks**  
- Detects sentence boundaries for natural pausing  
- Natural US English voice (22kHz 16-bit mono)  
- Error handling with retry logic  

**Format:** WAV files (temporary `/TTS_temp.wav`)

***

### Deep Sleep Mode
**Activation:** Automatic after **30 seconds** of inactivity  
**Wakeup:** Power cycle device  

**Behavior:**
- Audio announcement before sleep  
- Vibration feedback  
- All resources cleanly shut down  
- Servers stopped, BLE disconnected  

**Exempt Modes** (deep sleep disabled):
- B-DRIVE MODE  
- SD UPLOAD MODE  
- SD NAVIGATION MODE  
- SYSTEM UPDATE MODE  
- AUDIO PLAYER MODE  
- MODE OPTIONS  
- BOOT MODE  
- NORMAL MODE  

**Power Saving:** Conserves battery during extended idle periods

***

### OTA Firmware Update System
**Update Source:** GitHub repository  
**Current Version:** v2.0  
**Update Check:** Automatic when entering SYSTEM UPDATE MODE  

**Features:**
- Secure HTTPS download  
- Version comparison  
- Progress indicators  
- Automatic reboot after installation  
- 15-second confirmation timeout  

**Update Process:**
1. Fetches latest version info from GitHub  
2. Compares with current firmware  
3. Downloads `.bin` file if newer  
4. Writes to flash memory  
5. Verifies integrity  
6. Reboots device  

**Safety:** Cannot power off during update

***

### Braille Character Set

#### Letters
| Pattern | Dots | Char | Pattern | Dots | Char |
|---------|------|-------|----------|------|------|
| 1 | 1 | a | 5 | 1,3 | k |
| 3 | 1,2 | b | 7 | 1,2,3 | l |
| 9 | 1,4 | c | 13 | 1,3,4 | m |
| 25 | 1,4,5 | d | 29 | 1,3,4,5 | n |
| 17 | 1,5 | e | 21 | 1,3,5 | o |
| 11 | 1,2,4 | f | 15 | 1,2,3,4 | p |
| 27 | 1,2,4,5 | g | 31 | 1,2,3,4,5 | q |
| 19 | 1,2,5 | h | 23 | 1,2,3,5 | r |
| 10 | 2,4 | i | 14 | 2,3,4 | s |
| 26 | 2,4,5 | j | 30 | 2,3,4,5 | t |
| 37 | 1,3,6 | u | 39 | 1,2,3,6 | v |
| 58 | 2,4,5,6 | w | 45 | 1,3,4,6 | x |
| 61 | 1,3,4,5,6 | y | 53 | 1,3,5,6 | z |

#### Numbers (after pattern 60)
| Pattern | Number |
|----------|---------|
| 1 | 1 |
| 3 | 2 |
| 9 | 3 |
| 25 | 4 |
| 17 | 5 |
| 11 | 6 |
| 27 | 7 |
| 19 | 8 |
| 10 | 9 |
| 26 | 0 |

#### Punctuation
| Pattern | Symbol | Pattern | Symbol |
|----------|---------|----------|--------|
| 2 | , | 34 | . |
| 6 | ; | 38 | ! |
| 18 | : | 36 | ? |
| 20 | - | 44 | " |
| 8 | ' | 48 | = |
| 42 | ( | 50 | ) |

***

## SD Card File System

**Format:** FAT32  

**Organization:**
```
/NOTE1.txt, NOTE2.txt → Notes
/GEMINI1.txt, GEMINI2.txt → AI Responses
/Bdrive1.txt, Bdrive2.txt → OCR Outputs
/NOTIFY.txt → Notifications (cleared on boot)
/TACTI_VISION_WAV/ → Audio Feedback
  ├── PERKINS_MODE.wav
  ├── MODES.wav
  ├── SAVED.wav
  ├── SELECTED.wav
  ├── PREVIOUS.wav
  ├── ONE_HAND_MODE.wav
  ├── SOS.wav
  ├── Deepsleep.wav
  └── [other system sounds]
/AudioFiles/ → User audio files for Audio Player
/Alphabets/ → Letter pronunciation files (A.wav-Z.wav)
```

***

## Quick Reference Card

### Mode Access
| Action | Result |
|---------|---------|
| SELECT (short) | PERKINS MODE |
| SELECT (long) | MODE OPTIONS |
| PREVIOUS (long) | ONE-HAND MODE |
| **UP (long)** | **Toggle Alphabet Audio** |

### Common Actions
| Function | Input |
|-----------|--------|
| Type | Dots 1–6 |
| Space | Pin 8 / 9 |
| Delete | Pin 7 |
| Enter | Space + Backspace |
| Shift + Enter | Space + Ctrl |
| Save | Ctrl + Backspace |
| **Correct Text** | **Ctrl + Space** |
| **SOS Alert** | **Space (1.5s hold)** |
| Go Back | Pin 10 |

### Emergency Procedures
| Situation | Action |
|-----------|--------|
| Emergency | Hold Space L/R for 1.5s |
| Device Frozen | Power cycle (deep sleep) |
| Update Firmware | MODE OPTIONS → SYSTEM UPDATE |

***

## Technical Specifications

| Component | Specification |
|------------|----------------|
| MCU | ESP32C6 (dual-core, 160MHz) |
| GPIO Expander | PCF8575 (I2C address 0x20) |
| HID Interface | DigiSpark ATtiny85 (I2C address 0x23) |
| IMU | MPU6050 (I2C address 0x68) |
| BLE | ChronosESP32 |
| Audio | MAX98357A I2S amplifier |
| Storage | SD card (SPI, FAT32) |
| Power | 5V USB / Battery |
| **Deep Sleep** | **30s timeout, power cycle wakeup** |
| **I2C Bus** | **100kHz for multi-device stability** |

### Pin Configuration
| Function | Pin |
|----------|-----|
| I2C SDA | GPIO 0 |
| I2C SCL | GPIO 1 |
| Vibration | GPIO 2 |
| SD CS | GPIO 5 |
| I2S BCLK | GPIO 12 |
| I2S LRC | GPIO 13 |
| I2S DIN | GPIO 15 |
| SD MOSI | GPIO 22 |
| SD SCK | GPIO 23 |
| SD MISO | GPIO 21 |

***

## Support and Resources

### API Keys (Pre-configured)
- **Gemini AI:** Google Generative AI API  
- **VoiceRSS:** Text-to-speech API  
- **OCR.space:** Image text extraction  
- **Twilio:** SMS emergency alerts  

### Firmware Updates
- **Repository:** `github.com/MAATHES-THILAK-K/esp32-firmware-updates`  
- **Update Info:** `Firmware/latest.json`  
- **Current Version:** v2.0  

***

## About the Author

**Mᴀᴀᴛʜᴇs Tʜɪʟᴀᴋ K**  
Bachelor of Engineering – ECE  
Madras Institute of Technology (MIT), Anna University, Chennai  

**Interests & Aspirations**
- Embedded Systems Design  
- Robotics & Automation  
- Assistive Technology Development  

**Development Philosophy:**  
*"Vibe Coding — Leveraging AI as a collaborative tool to accelerate innovation and create meaningful accessibility solutions."*

***

### **Sᴛᴇʟʟᴀʀ Vɪsɪᴏɴ ᴠ1 – Making Technology Accessible**  
**Firmware Version:** 1.0  
**Last Updated:** November 2025  
**Developed by:** Mᴀᴀᴛʜᴇs Tʜɪʟᴀᴋ K  
