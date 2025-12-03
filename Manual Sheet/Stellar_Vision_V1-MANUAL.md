# **Sᴛᴇʟʟᴀʀ Vɪsɪᴏɴ ᴠ1 — USER MANUAL**
### *Assistive Braille Input Device with AI Integration*
### **Firmware Version: V3.2**

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
9. [Troubleshooting](#troubleshooting)
10. [Support and Resources](#support-and-resources)
11. [About the Author](#about-the-author)

***

## Device Overview

**Sᴛᴇʟʟᴀʀ Vɪsɪᴏɴ V1** is an assistive technology device designed for visually impaired users, featuring:

- **14-key Braille input system** via PCF8575 GPIO expander  
- **6-dot Braille pattern recognition** with full character set  
- **HID keyboard output** via DigiSpark (USB keyboard emulation)  
- **AI integration** (Gemini AI for queries)  
- **Notification system** via BLE (ChronosESP32)  
- **Audio feedback system** with voice confirmations and mode announcements  
- **SD card storage** for notes and files  
- **OCR capabilities** (*B-DRIVE mode* for text extraction from images/PDFs)  
- **Text-to-speech** for reading files aloud (300-character chunking)  
- **One-hand gesture mode** via MPU6050 accelerometer  
- **SOS Emergency Alert** via SMS (Twilio integration) with cooldown protection  
- **OTA firmware updates** via WiFi  
- **SD Upload Mode** with folder structure support  
- **Audio Player** for WAV file playback  
- **Alphabet audio feedback** for typed characters (toggle on/off)  
- **Text auto-correction** system (15+ common typos)  
- **Deep sleep mode** for power saving (30-second timeout)  
- **Enhanced I2C/I2S conflict management** for stable operation

***

## Button Layout

### Input Keys (14 Total)

| Pin | Function | Description |
|-----|-----------|-------------|
| 0–5 | Braille Dots 1–6 | Standard 6-dot Braille input |
| 6 | Ctrl Modifier | Toggle Ctrl key (for combos) |
| 7 | Backspace | Delete last character |
| 8 | Space (Left) | Space bar / Modifier / **SOS trigger (1.5s hold)** |
| 9 | Space (Right) | Space bar / Modifier / **SOS trigger (1.5s hold)** |
| 10 | Previous | Go back / Long press for gesture mode |
| 14 | Up | Navigate up in menus / **Long press (1s) toggles alphabet audio** |
| 12 | Select | Confirm / Enter mode |
| 13 | Down | Navigate down in menus |

***

## Getting Started

### Initial Boot
1. Power on the device  
2. Wait for initialization (vibration feedback)  
3. Device enters **NORMAL MODE** by default  
4. Serial output: *"Stellar Vision V1"*
5. Audio announcement plays

### First-Time Setup
- **SD Card**: Insert before powering on for file storage  
  - Create `/AudioFiles/` folder for audio player  
  - Create `/Alphabets/` folder for letter pronunciation (optional: A.wav through Z.wav)
  - Create `/TACTI_VISION_WAV/` folder for system audio feedback
- **BLE Connection**: Pair with mobile device for notifications  
- **HID Mode**: Connect DigiSpark for keyboard functionality  
- **WiFi Configuration**: Update WiFi credentials in firmware for AI and OTA features
- **Twilio SMS**: Configure credentials for SOS emergency alerts  

### Power Management
- Device automatically enters **deep sleep** after **30 seconds** of inactivity (in applicable modes)
- **Power cycle device to wake** from deep sleep
- Deep sleep announcement plays before entering sleep mode

***

## Operating Modes

### 1. NORMAL MODE
**Entry**: Device boots into this mode  
**Purpose**: Idle state, ready to enter other modes  

**Actions:**
- **Select (short press)** → Enter *PERKINS MODE*  
- **Select (long press 1s)** → Open *MODE OPTIONS*

**Power Management:**
- Deep sleep enabled after 30 seconds of inactivity
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
- **Ctrl (toggle)** → Toggle Ctrl modifier on/off  
- **Alphabet Audio** → Announces typed letters when enabled (toggle with UP long press)

#### Special Indicators
- **Pattern 60** (dots 3,4,5,6): Number mode indicator
- **Pattern 32** (dot 6): Capital mode indicator

#### Key Combinations
| Combination | Function |
|--------------|-----------|
| Space + Backspace (hold 400ms) | Enter key |
| Space + Ctrl (hold 400ms) | Shift + Enter (newline) |
| **Ctrl + Space (hold 400ms)** | **Apply text correction** |
| **Space LEFT or RIGHT (1.5s hold)** | **Send SOS emergency SMS** |
| **UP (long press 1s)** | **Toggle alphabet audio feedback** |
| **All 6 dots pressed** | **Toggle alphabet audio (alternative)** |

**HID Keyboard:**
- Automatically enabled when entering PERKINS MODE
- Requires DigiSpark connection
- Automatically disabled when leaving mode

**Exit:** Press PREVIOUS to return to MODE OPTIONS

***

### 3. MODE OPTIONS
**Entry:** Long press SELECT (1s) from any mode  
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
- **Up/Down:** Scroll through options (with audio announcements)
- **Select:** Choose highlighted option  
- **Previous:** Return to PERKINS MODE  

**Deep Sleep:** Disabled in this mode

***

### 4. NOTE-MAKER MODE
**Entry:** Select "NOTE-MAKER" from MODE OPTIONS  
**Purpose:** Create and save text notes to SD card  

#### Workflow
1. **Naming Phase**
   - Type desired filename (max 12 characters)  
   - Double-tap SELECT (within 500ms) to confirm name  
   - Double-tap SELECT again with no input for auto name (NOTE1.txt, NOTE2.txt…)
   - Names exceeding 12 characters will trigger error vibration

2. **Writing Phase**
   - Type note using Braille input  
   - BACKSPACE → Correct errors  
   - SPACE → Insert space  
   - **Ctrl + Space (hold 400ms)** → Apply text correction

3. **Saving**
   - **Ctrl + Backspace (hold 400ms)** → Save and exit  
   - Audio feedback confirms save  
   - Returns to MODE OPTIONS

**Exit:** PREVIOUS before saving → Discard note and return to MODE OPTIONS

***

### 5. SD CARD MODE
**Entry:** Select "SD CARD" from MODE OPTIONS  
**Purpose:** Browse and read files on SD card  

**Navigation:**
- **Up/Down:** Navigate file list (sorted alphabetically)
- **Select:** Display file content (printed to serial)
- **Ctrl + Backspace (hold 400ms):** Text-to-speech (TTS) of current file

**TTS Features:**
- Reads files in **300-character chunks** for natural pacing
- Detects sentence boundaries (periods, exclamation marks, questions, newlines)
- Announces conversion process with audio feedback
- Automatic cleanup of temporary files
- Success/failure vibration feedback

**File Compatibility:**  
`.txt` (UTF-8 text)  

**Power Management:** Deep sleep is **disabled** in this mode

**Exit:** Press PREVIOUS  

***

### 6. GEMINI AI MODE
**Entry:** Select "GEMINI AI" from MODE OPTIONS  
**Purpose:** Query Google Gemini AI and save responses  

#### Workflow
1. **WiFi Connection:** Device auto-connects on mode entry
2. **Query Phase:** Type your question in Braille  
3. **Send Query:** Ctrl + Backspace (hold 400ms)
   - Audio announcement: "Sending Query"
   - Response received announcement
4. **Save Response:**
   - Press SELECT to start naming process
   - Type filename (max 12 chars)
   - Double-tap SELECT to confirm
   - Double-tap SELECT with no input for default name (GEMINI1.txt, GEMINI2.txt…)
5. **Complete Save:** Ctrl + Backspace to finalize

**Audio Feedback:**
- WiFi connection status
- Query sending confirmation
- Response received notification

**Requirements:**  
- WiFi connection (auto-connects on mode entry)  
- Internet access  
- Pre-configured Gemini API key  

**Text Correction:** Available during query typing (Ctrl + Space)

**Exit:** PREVIOUS → Returns to MODE OPTIONS

***

### 7. B-DRIVE MODE
**Entry:** Select "B-DRIVE" from MODE OPTIONS  
**Purpose:** Extract text from images/PDFs via OCR  

#### Setup
1. Device announces local web server IP (port 80)  
2. Connect device to same WiFi network  
3. Access via browser at `http://[IP_ADDRESS]`  

#### Web Interface Features
- Modern, responsive design with gradient background
- Drag-and-drop file upload
- Upload progress indicator
- Real-time status updates
- Support for multiple file types

#### Supported Formats
- **Images:** JPG, PNG (via OCR.space API)
- **PDFs:** Text extraction (client-side JavaScript processing)

#### Workflow
1. Upload file via web interface (drag-and-drop or browse)
2. **For Images:**
   - Sent to OCR.space API for processing
   - Device vibrates when OCR complete
3. **For PDFs:**
   - JavaScript extracts text directly in browser
   - Text sent to device automatically
4. **Extracted text displayed** on serial output
5. **Save Process:**
   - Press SELECT to start naming
   - Type filename (max 12 chars)
   - Double-tap SELECT to confirm name
   - Press Ctrl + Backspace to save

**Audio Feedback:**
- Mode entry announcement
- OCR completion notification
- Save confirmation

**Requirements:**
- WiFi connection + OCR.space API key  
- SD card for file storage

**Power Management:** Deep sleep is **disabled** in this mode

**Exit:** PREVIOUS (stops web server and returns to MODE OPTIONS)

***

### 8. HID SHORTCUT MODE
**Entry:** Select "HID SHORTCUT" from MODE OPTIONS  
**Purpose:** Execute predefined keyboard shortcuts on connected computer

#### Available Shortcuts
| Shortcut | Action | Audio File |
|-----------|--------|------------|
| WIFI | Open WiFi settings | WIFI.wav |
| CHROME | Launch Chrome browser | CHROME.wav |
| GMAIL | Open Gmail | GMAIL.wav |
| CHATGPT | Open ChatGPT | CHATGPT.wav |
| PY COMPILER | Open Python IDE | PY_COMPILER.wav |
| LIBRARY | Open Library app | LIBRARY.wav |

**Navigation:**
- **Up/Down:** Browse shortcuts (with audio announcements)
- **Select:** Execute highlighted shortcut  

**Execution:**
- Success: Audio confirmation + vibration (0.3s)
- Failure: Error vibration (0.1s)

**Requirements:**  
- DigiSpark HID connected and initialized
- Compatible operating system (Windows/Mac/Linux)

**Exit:** PREVIOUS → Returns to MODE OPTIONS

***

### 9. NOTIFY MODE
**Entry:** Select "NOTIFY" from MODE OPTIONS  
**Purpose:** Receive and log smartphone notifications via BLE

**Activation:**
- First SELECT → Turn ON (vibration feedback + "NOTIFY is ON")
- Second SELECT → Turn OFF (vibration feedback + "NOTIFY is OFF")

**Features:**
- BLE connection via ChronosESP32  
- Logs notifications to `/NOTIFY.txt`  
- Ring buffer (8 messages maximum)
- Processes notifications from background

**Log Format:**
```
App: [app_name] [timestamp]
Msg: [title] [message]
---------------------
```

**Behavior:**
- Notifications received only when NOTIFY is ON
- Old `/NOTIFY.txt` cleared on each boot
- Automatic connection callbacks for device pairing

**Connection Feedback:**
- Vibration on mobile connection/disconnection
- Serial output for connection status

**Exit:** SELECT again to toggle OFF, or PREVIOUS to return to MODE OPTIONS

***

### 10. SD UPLOAD MODE
**Entry:** Select "SD UPLOAD" from MODE OPTIONS  
**Purpose:** Upload files and folders wirelessly to SD card  

#### Features
- Web-based file upload interface (port 8080)
- **Full folder structure support** with drag-and-drop  
- Multiple file upload capability
- Browse, download, and delete files  
- Directory tree visualization  
- Real-time progress indicators

#### Setup
1. Device announces IP address on port 8080  
2. Access `http://[IP_ADDRESS]:8080` in browser  
3. Modern purple gradient interface loads

#### File Management
- **Upload Files:** Drag-and-drop or click to browse
- **Upload Folders:** Maintains complete directory structure
- **View Files:** List with file sizes and types
- **Download:** Click download button on any file
- **Delete:** Remove files remotely with confirmation
- **Create Directories:** Automatically created when uploading folders

#### Upload Process
1. **Single Files:**
   - Select or drag files to upload area
   - Progress bar shows upload status
   - Success/error status displayed

2. **Folder Upload:**
   - Select entire folder (maintains structure)
   - Creates all subdirectories automatically
   - Uploads all files preserving paths
   - Progress indicator for batch upload

**Audio Feedback:**
- Mode entry announcement
- File upload completion sound
- Ready to receive notification

**Requirements:**
- WiFi connection  
- SD card with sufficient space

**Power Management:** Deep sleep is **disabled** in this mode

**Exit:** PREVIOUS (stops server, returns to MODE OPTIONS)

***

### 11. SYSTEM UPDATE MODE
**Entry:** Select "SYSTEM UPDATE" from MODE OPTIONS  
**Purpose:** Check for and install firmware updates via OTA (Over-The-Air)

#### Workflow
1. **Update Check:**
   - Device connects to GitHub repository
   - Compares current firmware (V3.2) with latest version
   - Audio announcement: "Firmware Check"

2. **No Update Available:**
   - Message: "You're running the latest version"
   - Audio: "Latest Version"
   - Returns to MODE OPTIONS after 2 seconds

3. **Update Available:**
   - Announces new version number
   - Audio: "Firmware Available"
   - Displays: "Press SELECT to update" or "Press PREVIOUS to skip"
   - **15-second timeout** for user confirmation

4. **Installation Process:**
   - Audio: "Downloading"
   - Progress indicators via vibration
   - ⚠️ **CRITICAL: DO NOT POWER OFF** during update
   - Automatic verification of downloaded firmware
   - Success audio: "Update Success"
   - **Device reboots automatically** after successful update

5. **Update Cancellation:**
   - Press PREVIOUS before timeout
   - Returns to MODE OPTIONS

**Safety Features:**
- HTTPS secure download
- Firmware integrity verification
- Automatic rollback on failure
- Timeout protection

**Update Source:**
- Repository: `github.com/MAATHES-THILAK-K/Stellar_Vision_V1`
- Update info: `Firmware/latest.json`

**Power Management:** Deep sleep is **disabled** in this mode

**Exit:** PREVIOUS (cancels update and returns to MODE OPTIONS)

***

### 12. AUDIO PLAYER MODE
**Entry:** Select "AUDIO PLAYER" from MODE OPTIONS  
**Purpose:** Play WAV audio files from SD card  

#### Setup
- Place `.wav` files in `/AudioFiles/` folder on SD card  
- Files automatically sorted alphabetically  
- Recommended format: 22kHz 16-bit mono WAV

#### Controls
| Button | Function | Behavior |
|---------|----------|----------|
| **UP** | Previous track | Stops current playback, moves to previous file |
| **DOWN** | Next track | Stops current playback, moves to next file |
| **SELECT** | Play/Stop | Toggle playback of current track |
| **PREVIOUS** | Exit mode | Stops playback and returns to MODE OPTIONS |

#### Playback Features
- Audio feedback for track changes  
- File list announced on mode entry  
- Current track displayed on serial output
- Smooth transitions between tracks
- Automatic cleanup after playback

#### File Management
- If `/AudioFiles/` folder doesn't exist, it's created automatically
- Supports standard WAV format
- No playlist saving (plays files in alphabetical order)

**Requirements:**
- SD card with `/AudioFiles/` folder  
- WAV audio files (other formats not supported)

**Audio Feedback:**
- Mode entry: "Selected"
- Initialization: "Audio Init"
- Playback stopped: "Playback Stopped"

**Power Management:** Deep sleep is **disabled** in this mode

**Exit:** PREVIOUS (stops any active playback and returns to MODE OPTIONS)

***

## Navigation Guide

### Global Button Combinations

| Combination | Function | Hold Duration |
|--------------|-----------|---------------|
| Ctrl + Backspace | Save / Send / TTS | 400ms |
| Space + Backspace | Enter | 400ms |
| Space + Ctrl | Shift + Enter | 400ms |
| **Ctrl + Space** | **Apply text correction** | **400ms** |
| **Space LEFT (hold)** | **SOS Emergency SMS** | **1500ms** |
| **Space RIGHT (hold)** | **SOS Emergency SMS** | **1500ms** |
| SELECT (long press) | Open MODE OPTIONS | 1000ms |
| PREVIOUS (long press) | Toggle ONE-HAND MODE | 1000ms |
| **UP (long press)** | **Toggle alphabet audio** | **1000ms** |
| **All 6 dots pressed** | **Toggle alphabet audio** | **Instant** |

### Mode Transitions

```
NORMAL MODE  
↓ (SELECT short press)  
PERKINS MODE  
↓ (SELECT long press 1s)  
MODE OPTIONS  
↓ (SELECT on option)  
[Selected Mode]  
↓ (PREVIOUS)  
MODE OPTIONS  
↓ (PREVIOUS)  
PERKINS MODE  
```

### Deep Sleep Behavior

**Modes with Deep Sleep Enabled (30s timeout):**
- NORMAL MODE
- PERKINS MODE

**Modes with Deep Sleep Disabled:**
- MODE OPTIONS
- B-DRIVE MODE
- SD UPLOAD MODE
- SD NAVIGATION MODE
- SYSTEM UPDATE MODE
- AUDIO PLAYER MODE
- BOOT MODE

***

## Advanced Features

### ONE-HAND GESTURE MODE
**Activation:** Long press PREVIOUS (1000ms) from PERKINS, NORMAL, or MODE OPTIONS
**Purpose:** Control device via MPU6050 accelerometer tilts

#### Tilt Actions
| Tilt Direction | Action | Debounce |
|----------------|--------|----------|
| Right | SELECT | 450ms |
| Left | PREVIOUS | 450ms |
| Up | UP navigation | 450ms |
| Down | DOWN navigation | 450ms |

#### Calibration
Uses pre-configured offsets for accurate tilt detection:
- **Acceleration X:** 0.88
- **Acceleration Y:** -0.20
- **Acceleration Z:** -10.03
- **Tilt Threshold:** 3.0

**Disable:** Long press PREVIOUS again

**Audio:** "One Hand Mode" announcement on activation

***

### SOS Emergency Alert System
**Activation:** Long press **either SPACE button** for **1.5 seconds**  
**Provider:** Twilio SMS API  

#### Features
- Sends emergency SMS to pre-configured number  
- Includes device status and uptime  
- **30-second cooldown** between alerts (prevents spam)
- Audio announcement: `/TACTI_VISION_WAV/SOS.wav`  
- Works from any mode

#### SMS Content
```
🚨 SOS TRIGGERED FROM STELLAR VISION
STATUS: Emergency
DEVICE: TACTI-WAVE
UPTIME: [hours]h [minutes]m [seconds]s
⚠️ IMMEDIATE ASSISTANCE REQUIRED
```

#### Behavior
1. **First Trigger:**
   - WiFi auto-connects if needed
   - SMS sent immediately
   - Success vibration (0.5s)
   - Cooldown timer starts

2. **During Cooldown:**
   - Shows remaining time (in seconds)
   - Error vibration (0.1s)
   - Message: "Cooldown active"

3. **After Cooldown:**
   - Full functionality restored
   - New SMS can be sent

**Requirements:**
- WiFi connection (auto-connects if needed)  
- Twilio account credentials configured  
- Valid phone numbers in international format

**Troubleshooting:**
- 401 Error: Invalid credentials (check SID/Token)
- 400 Error: Invalid phone number format
- 500+ Error: Twilio server issue (retry)

***

### Text Auto-Correction System
**Activation:** Ctrl + Space (hold 400ms)  
**Purpose:** Correct common typing mistakes automatically

#### Supported Corrections (15 Total)
| Typo | Correction | Typo | Correction |
|------|------------|------|------------|
| teh | the | heelp | help |
| pleese | please | cal | call |
| tnanks | thanks | fone | phone |
| mesage | message | reed | read |
| adio | audio | braile | braille |
| scren | screen | voise | voice |
| setings | settings | batery | battery |
| emergncy | emergency | | |

#### How It Works
1. Scans `currentWord` for known typos
2. Identifies word boundaries (space, punctuation, newlines)
3. Replaces typos with correct spellings
4. Maintains capitalization and punctuation
5. Counts and reports corrections made

**Available In:**
- PERKINS MODE
- NOTE-MAKER MODE
- GEMINI AI MODE (query typing)
- B-DRIVE MODE (naming phase)
- GEMINI MODE (naming phase)

**Feedback:**
- Vibration (0.2s) on successful correction
- Audio announcement: "Autocorrection"
- Serial output shows original and corrected text
- Correction count displayed

**Process:**
```
Original Text:
"pleese cal me if you need heelp with the setings"

Corrected Text:
"please call me if you need help with the settings"

[CORRECTION] Made 4 correction(s)
```

***

### Alphabet Audio Feedback
**Toggle Methods:**
1. Long press UP (1000ms)  
2. Press all 6 Braille dots simultaneously

**Audio Source:** `/Alphabets/A.wav` through `/Alphabets/Z.wav`  

#### Features
- Announces each letter as you type  
- Works with both lowercase and uppercase  
- **Disabled by default** (to save processing time)  
- Available in all typing modes (PERKINS, NOTE-MAKER, GEMINI AI, B-DRIVE naming)
- Plays after character is typed

#### File Requirements
- 26 files named: A.wav, B.wav, C.wav... Z.wav
- Located in `/Alphabets/` folder
- Recommended format: 22kHz 16-bit mono WAV
- Short duration (0.5-1 second recommended)

**Audio Announcements:**
- Enable: "Alpha_enab.wav" + vibration (0.3s)
- Disable: "Alpha_disable.wav" + vibration (0.2s)

**Behavior:**
- Automatically converts lowercase to uppercase for file lookup
- Silent if file missing (no error)
- Only triggers for letters A-Z

***

### Audio Feedback System
**Hardware:** MAX98357A I2S amplifier  

#### System Features
- Mode entry notifications  
- Action confirmations  
- Gesture mode announcements  
- Alphabet pronunciation (when enabled)  
- Deep sleep announcement  
- SOS alert sound  
- Update process feedback

#### I2C/I2S Conflict Prevention
**Problem:** ESP32C6 I2C and I2S share resources  
**Solution:**
1. **Pre-Audio Preparation:**
   - Temporarily disables HID keyboard
   - Allows pending I2C transactions to complete (50ms delay)
   
2. **Post-Audio Cleanup:**
   - Stops I2S output completely
   - Ends I2C bus (Wire.end())
   - Waits 150ms for hardware reset
   
3. **I2C Reinit:**
   - Restarts I2C at 100kHz for stability
   - Re-tests PCF8575 connection
   - Checks DigiSpark availability
   - Re-enables HID if available

4. **Device Recovery:**
   - Automatic DigiSpark reconnection (with retry)
   - Error handling for failed reconnections
   - Status reporting via serial output

**I2C Configuration:**
- **Bus Speed:** 100kHz (reduced from 400kHz for multi-device stability)
- **Timeout:** 500ms per transaction
- **Devices:** PCF8575 (0x20), DigiSpark (0x23), MPU6050 (0x68)

***

### Text-to-Speech (TTS)
**Trigger:** Ctrl + Backspace (in SD CARD MODE)  
**Provider:** VoiceRSS API  

#### Features
- Reads text aloud in **300-character chunks**  
- Detects sentence boundaries for natural pausing  
- Natural US English voice  
- 22kHz 16-bit mono WAV output  
- Error handling with visual/haptic feedback  

#### Chunking Logic
1. Divides file into 300-character segments
2. Finds last sentence-ending punctuation (. ! ? or newline)
3. If found in second half of chunk, breaks there for natural pacing
4. Otherwise, breaks at 300-character mark

#### Process
1. Audio: "Converting TTS"
2. Calculates total chunks needed
3. Processes each chunk:
   - Fetches TTS from VoiceRSS
   - Saves to `/TTS_temp.wav`
   - Plays audio immediately
   - Deletes temp file
   - 300ms pause between chunks
4. Final vibration feedback (success or error)

**Success/Failure:**
- All chunks successful: Vibration (0.25s)
- Some failures: Error vibration (0.15s)
- Serial output shows success/failure count

**Format:** Temporary WAV files (`/TTS_temp.wav`) auto-deleted

***

### Deep Sleep Mode
**Activation:** Automatic after **30 seconds** of inactivity  
**Wakeup:** **Power cycle device**  

#### Sleep Process
1. **Audio Announcement:** "Deepsleep.wav" plays first
2. **Delay:** 1000ms for audio completion
3. **Pre-Sleep Vibration:** 0.5s haptic feedback
4. **Vibration Complete:** 600ms delay ensures motor stops
5. **Resource Cleanup:**
   - Stops B-DRIVE server (if active)
   - Stops SD UPLOAD server (if active)
   - Disconnects BLE notifications (if active)
   - Cleans up audio resources
6. **Motor Safeguard:**
   - Vibration motor set to LOW
   - Pin reconfigured to INPUT_PULLDOWN (prevents floating)
7. **Enter Sleep:** `esp_deep_sleep_start()`

#### Exempt Modes (Sleep Disabled)
- **B-DRIVE MODE** (web server active)
- **SD UPLOAD MODE** (file transfer active)
- **SD NAVIGATION MODE** (user browsing files)
- **SYSTEM UPDATE MODE** (OTA in progress)
- **AUDIO PLAYER MODE** (audio playback)
- **MODE OPTIONS** (user navigating)
- **BOOT MODE** (initialization)
- **NORMAL MODE** (ready state)

**Activity Timer:**
- Reset on any button press
- Reset on any Braille input
- Reset on mode change
- 30-second countdown in applicable modes

**Power Saving:** Reduces battery consumption by ~90% during idle periods

**Important:** No wakeup source configured - requires physical power cycle

***

### OTA Firmware Update System
**Update Source:** GitHub repository  
**Current Version:** V3.2  
**Update Check:** Automatic when entering SYSTEM UPDATE MODE  

#### Update Process
1. **Version Check:**
   - Fetches `latest.json` from GitHub
   - Compares with current V3.2
   - HTTPS secure connection

2. **User Confirmation:**
   - Displays new version number
   - 15-second timeout for decision
   - SELECT confirms, PREVIOUS cancels

3. **Download:**
   - Audio: "Downloading"
   - Fetches `.bin` file from GitHub
   - Progress monitoring
   - Size verification

4. **Installation:**
   - Writes to flash memory
   - Integrity verification
   - Error handling with automatic abort

5. **Completion:**
   - Audio: "Update Success"
   - Vibration feedback (0.8s)
   - Automatic reboot (2-second delay)

#### Safety Features
- **DO NOT POWER OFF** warning
- Secure HTTPS download only
- Firmware signature verification
- Automatic rollback on failure
- No partial updates (all-or-nothing)

**Update Info File (latest.json):**
```json
{
  "latest_version": "V3.3",
  "url": "https://github.com/.../firmware_v3.3.bin"
}
```

**Timeout Handling:**
- 15 seconds to confirm update
- Auto-cancels if no response
- Returns to MODE OPTIONS

***

### Braille Character Set

#### Letters (Lowercase Default)
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

#### Numbers (Preceded by Pattern 60)
| Pattern | After [NUM] | Number |
|----------|-------------|---------|
| 1 | 1 | 1 |
| 3 | 1,2 | 2 |
| 9 | 1,4 | 3 |
| 25 | 1,4,5 | 4 |
| 17 | 1,5 | 5 |
| 11 | 1,2,4 | 6 |
| 27 | 1,2,4,5 | 7 |
| 19 | 1,2,5 | 8 |
| 10 | 2,4 | 9 |
| 26 | 2,4,5 | 0 |

#### Punctuation
| Pattern | Dots | Symbol | Pattern | Dots | Symbol |
|----------|------|---------|----------|------|--------|
| 2 | 2 | , | 34 | 2,5,6 | . |
| 6 | 2,3 | ; | 38 | 2,3,5 | ! |
| 18 | 1,2,5 | : | 36 | 2,6 | ? |
| 20 | 3,6 | - | 44 | 5 | " |
| 8 | 3 | ' | 48 | 4,6 | = |
| 42 | 1,2,6 | ( | 50 | 3,4,5 | ) |

**Mode Indicators:**
- Pattern 60 (3,4,5,6): Activates number mode for next character
- Pattern 32 (6): Activates capital mode for next character

***

## SD Card File System

**Format:** FAT32  
**Organization:**

```
/
├── NOTE1.txt, NOTE2.txt, ...     → User notes from NOTE-MAKER
├── GEMINI1.txt, GEMINI2.txt, ... → AI responses from GEMINI AI
├── Bdrive1.txt, Bdrive2.txt, ... → OCR outputs from B-DRIVE
├── NOTIFY.txt                     → BLE notifications (cleared on boot)
├── TTS_temp.wav                   → Temporary TTS file (auto-deleted)
│
├── /TACTI_VISION_WAV/            → System audio feedback
│   ├── StellarVision.wav         → Boot announcement
│   ├── PERKINS_MODE.wav          → Mode entry sounds
│   ├── MODES.wav
│   ├── SAVED.wav
│   ├── SELECTED.wav
│   ├── PREVIOUS.wav
│   ├── ONE_HAND_MODE.wav
│   ├── SOS.wav                   → Emergency alert sound
│   ├── Deepsleep.wav             → Sleep announcement
│   ├── Alpha_enab.wav            → Alphabet audio enabled
│   ├── Alpha_disable.wav         → Alphabet audio disabled
│   ├── Autocorrection.wav        → Text correction sound
│   ├── OCR_Completed.wav         → B-DRIVE OCR done
│   ├── File_Upload.wav           → SD upload success
│   ├── converting_TTS.wav        → TTS conversion start
│   ├── hold_save.wav             → Save instruction
│   ├── hold_query.wav            → Gemini query instruction
│   ├── Sending_Query.wav         → Gemini sending
│   ├── response_received.wav     → Gemini response ready
│   ├── Connecting_wifi.wav       → WiFi connecting
│   ├── wificonnected.wav         → WiFi success
│   ├── Firmware_check.wav        → OTA check start
│   ├── firmware_available.wav    → Update available
│   ├── Downloading.wav           → Firmware download
│   ├── update_sucess.wav         → Update complete
│   ├── latestversion.wav         → Already up to date
│   ├── READY_TO_RECEIVE.wav      → SD upload ready
│   ├── Audio_Init.wav            → Audio player init
│   ├── playbackstopped.wav       → Audio stopped
│   └── [other system sounds]
│
├── /AudioFiles/                  → User audio files (AUDIO PLAYER)
│   ├── song1.wav
│   ├── podcast.wav
│   └── ...
│
├── /Alphabets/                   → Letter pronunciation (optional)
│   ├── A.wav
│   ├── B.wav
│   ├── ...
│   └── Z.wav
│
└── /[user-created-folders]/      → SD UPLOAD folder structure
    └── [nested directories]
```

***

## Quick Reference Card

### Mode Access
| Action | Result | Hold Time |
|---------|---------|-----------|
| SELECT (short) | PERKINS MODE | < 1s |
| SELECT (long) | MODE OPTIONS | 1s |
| PREVIOUS (long) | ONE-HAND MODE | 1s |
| **UP (long)** | **Toggle Alphabet Audio** | **1s** |

### Common Actions
| Function | Input | Hold Time |
|-----------|--------|-----------|
| Type | Dots 1–6 | - |
| Space | Pin 8 or 9 | - |
| Delete | Pin 7 | - |
| Enter | Space + Backspace | 400ms |
| Shift + Enter | Space + Ctrl | 400ms |
| Save/Send/TTS | Ctrl + Backspace | 400ms |
| **Correct Text** | **Ctrl + Space** | **400ms** |
| **SOS Alert** | **Space (either)** | **1500ms** |
| Go Back | Pin 10 | < 1s |
| Navigate | UP/DOWN | - |
| Confirm | SELECT | < 1s |

### Emergency Procedures
| Situation | Action | Notes |
|-----------|--------|-------|
| **Emergency** | **Hold either Space 1.5s** | **30s cooldown between alerts** |
| Device Frozen | Power cycle | Wakes from deep sleep |
| Update Firmware | MODE OPTIONS → SYSTEM UPDATE | V3.2 → latest |
| Reset Audio | Power cycle | Reinits I2C/I2S |

***

## Technical Specifications

### Hardware
| Component | Specification | Address/Pin |
|------------|----------------|-------------|
| MCU | ESP32C6 (dual-core, 160MHz) | - |
| GPIO Expander | PCF8575 (16-bit I/O) | I2C 0x20 |
| HID Interface | DigiSpark ATtiny85 | I2C 0x23 |
| IMU | MPU6050 (6-axis) | I2C 0x68 |
| BLE | ChronosESP32 | Built-in |
| Audio | MAX98357A I2S amplifier | I2S |
| Storage | SD card (FAT32, SPI) | SPI |
| Power | 5V USB / Battery | - |
| Vibration Motor | DC motor | GPIO 2 |

### Pin Configuration
| Function | GPIO Pin | Protocol |
|----------|----------|----------|
| I2C SDA | 0 | I2C (100kHz) |
| I2C SCL | 1 | I2C (100kHz) |
| Vibration Motor | 2 | Digital Out |
| SD CS | 5 | SPI |
| I2S BCLK | 12 | I2S |
| I2S LRC (WS) | 13 | I2S |
| I2S DIN (SD) | 15 | I2S |
| SD MISO | 21 | SPI |
| SD MOSI | 22 | SPI |
| SD SCK | 23 | SPI |

### Software
| Feature | Details |
|---------|---------|
| **Firmware Version** | **V3.2** |
| I2C Bus Speed | 100kHz (multi-device stability) |
| I2C Timeout | 500ms per transaction |
| Deep Sleep Timeout | 30 seconds |
| Braille Input Delay | 150ms character recognition |
| Button Debounce | 50ms |
| Combo Hold Duration | 400ms |
| Long Press Duration | 1000ms |
| **SOS Trigger Time** | **1500ms** |
| **SOS Cooldown** | **30 seconds** |
| TTS Chunk Size | 300 characters |

### Power Management
| Mode | Deep Sleep | Timeout |
|------|-----------|---------|
| NORMAL | Enabled | 30s |
| PERKINS | Enabled | 30s |
| MODE OPTIONS | Disabled | N/A |
| B-DRIVE | Disabled | N/A |
| SD UPLOAD | Disabled | N/A |
| SD NAVIGATION | Disabled | N/A |
| SYSTEM UPDATE | Disabled | N/A |
| AUDIO PLAYER | Disabled | N/A |

### Network Features
| Service | Port | Protocol |
|---------|------|----------|
| B-DRIVE Web Server | 80 | HTTP |
| SD Upload Server | 8080 | HTTP |
| OTA Updates | 443 | HTTPS |
| Twilio SMS | 443 | HTTPS |
| VoiceRSS TTS | 80/443 | HTTP/HTTPS |
| OCR.space API | 443 | HTTPS |
| Gemini AI API | 443 | HTTPS |

***

## Troubleshooting

### Device Issues

#### Device Not Responding
**Symptoms:** No vibration, no audio, buttons not working  
**Solutions:**
1. Power cycle the device
2. Check battery/USB power connection
3. Verify SD card is properly inserted
4. Wait 30 seconds (may be in deep sleep)

#### HID Keyboard Not Working
**Symptoms:** Typing in PERKINS MODE doesn't appear on computer  
**Checks:**
- DigiSpark connected? (check USB connection)
- Serial shows "✅ DigiSpark found"? (if not, reconnect USB)
- Correct USB port? (try different port)
- Driver installed? (DigiSpark requires drivers on first use)

**Fix:**
1. Exit to MODE OPTIONS
2. Re-enter PERKINS MODE (reinitializes HID)
3. If still failing, power cycle device

#### Audio Not Playing
**Symptoms:** Silent operation, no mode announcements  
**Checks:**
- SD card inserted and readable?
- `/TACTI_VISION_WAV/` folder exists?
- WAV files present? (check file list)
- Speaker/amplifier connected?

**I2C/I2S Conflict:**
- Audio temporarily disables I2C devices
- Automatic recovery after playback
- If frozen after audio, power cycle

#### Vibration Motor Stuck
**Symptoms:** Vibration continues indefinitely  
**Solution:**
- Power cycle device immediately
- Motor safeguard should prevent this in V3.2

### Mode-Specific Issues

#### GEMINI AI Not Responding
**Symptoms:** No response after query, timeout errors  
**Checks:**
- WiFi connected? (check serial output)
- Internet access working?
- API key valid? (check firmware config)
- Query too long? (try shorter query)

**Fix:**
1. Exit mode (PREVIOUS)
2. Check WiFi signal strength
3. Re-enter GEMINI AI MODE
4. Try simple test query: "hello"

#### B-DRIVE OCR Failing
**Symptoms:** "No text detected" or timeout  
**Checks:**
- WiFi connected?
- OCR.space API key valid?
- Image quality good? (clear text, good lighting)
- File size < 1MB?

**Fix:**
1. Verify image is readable (not upside-down)
2. Try different image format (JPG recommended)
3. Reduce file size if large
4. Check API quota (OCR.space limits)

#### SD UPLOAD Not Accessible
**Symptoms:** Cannot access web interface  
**Checks:**
- WiFi connected on computer and device?
- Same network? (check IP address)
- Port 8080 open? (firewall settings)
- Browser shows correct URL: `http://[IP]:8080`?

**Fix:**
1. Note IP address announced by device
2. Disable firewall temporarily
3. Try different browser
4. Re-enter SD UPLOAD MODE

#### TTS Not Speaking
**Symptoms:** Silent file reading, conversion fails  
**Checks:**
- WiFi connected?
- VoiceRSS API key valid?
- File format supported? (only .txt)
- File not empty?

**Fix:**
1. Check file content (SELECT to print)
2. Verify internet connection
3. Try smaller file (<10KB for testing)
4. Check VoiceRSS API quota

#### AUDIO PLAYER No Files
**Symptoms:** "No .wav files found"  
**Checks:**
- `/AudioFiles/` folder exists?
- Files are .wav format? (not .mp3, .ogg, etc.)
- File names correct? (case-sensitive)
- SD card readable?

**Fix:**
1. Create `/AudioFiles/` folder if missing
2. Add .wav files (22kHz 16-bit mono recommended)
3. Re-enter AUDIO PLAYER MODE
4. Check file list on serial output

### OTA Update Issues

#### Update Check Fails
**Symptoms:** "Failed to fetch update info"  
**Checks:**
- WiFi connected?
- GitHub accessible? (check internet)
- Timeout (15s for connection)?

**Fix:**
1. Exit and re-enter SYSTEM UPDATE MODE
2. Check WiFi signal strength
3. Try later (GitHub may be down)

#### Update Download Fails
**Symptoms:** Error during firmware download  
**Checks:**
- WiFi stable? (don't move device)
- Sufficient space? (unlikely)
- Power stable? (don't disconnect USB)

**Fix:**
- **DO NOT power off during update**
- Wait for automatic retry
- If fails 3 times, exit and try later

#### Device Won't Boot After Update
**Symptoms:** No response after reboot  
**Solution:**
- Power cycle 2-3 times
- Check for continuous reboot loop
- May need to reflash firmware via USB (rare)

### Deep Sleep Issues

#### Device Sleeps Too Quickly
**Symptoms:** Enters sleep during use  
**Cause:** 30-second timeout in certain modes  
**Solution:**
- Keep interacting with device (any button press resets timer)
- Only NORMAL and PERKINS modes have sleep timeout

#### Can't Wake from Sleep
**Symptoms:** Device unresponsive after sleep  
**Solution:**
- **Power cycle required** (no button wake)
- Disconnect and reconnect USB power
- Or press physical reset button if available

#### Didn't Sleep When Expected
**Cause:** Currently in exempt mode  
**Modes Without Sleep:**
- MODE OPTIONS (navigating)
- B-DRIVE (server running)
- SD UPLOAD (server running)
- All active modes

### Notification Issues

#### BLE Not Connecting
**Symptoms:** No notifications from phone  
**Checks:**
- ChronosESP32 app installed on phone?
- Bluetooth enabled on phone?
- NOTIFY mode turned ON? (check serial)
- Paired correctly?

**Fix:**
1. Toggle NOTIFY OFF then ON
2. Restart ChronosESP32 app
3. Re-pair Bluetooth on phone
4. Power cycle device

#### Notifications Not Logging
**Symptoms:** NOTIFY.txt empty or missing  
**Checks:**
- SD card working?
- NOTIFY mode ON? (must be enabled)
- Ring buffer full? (8-message limit)
- `/NOTIFY.txt` cleared on boot (normal behavior)

### General Performance

#### Sluggish Response
**Causes:**
- SD card slow/corrupted
- WiFi interference
- Too many devices on I2C bus

**Solutions:**
1. Use faster SD card (Class 10 recommended)
2. Reduce WiFi distance to router
3. Power cycle to reset I2C bus
4. Check for loose connections

#### Random Reboots
**Causes:**
- Power supply insufficient
- SD card issues
- Memory overflow (rare)

**Solutions:**
1. Use quality USB power adapter (5V 2A minimum)
2. Check SD card health
3. Reflash firmware if persistent

***

## Support and Resources

### API Keys (Pre-configured in Firmware)
- **Gemini AI:** Google Generative AI API  
- **VoiceRSS:** Text-to-speech API  
- **OCR.space:** Image text extraction  
- **Twilio:** SMS emergency alerts  

**⚠️ Security Note:** API keys are hardcoded in firmware. For production, use environment variables or secure storage.

### Firmware Updates
- **Repository:** `github.com/MAATHES-THILAK-K/Stellar_Vision_V1`  
- **Update Info:** `Firmware/latest.json`  
- **Current Version:** V3.2  
- **Update Method:** OTA (Over-The-Air) via SYSTEM UPDATE MODE

### Required SD Card Structure
```
/TACTI_VISION_WAV/       → System audio (required)
/AudioFiles/             → User audio (optional)
/Alphabets/              → Letter sounds (optional)
```

### Calibration Values (MPU6050)
Located in firmware for gesture mode:
- ACCEL_X_OFFSET: 0.88
- ACCEL_Y_OFFSET: -0.20
- ACCEL_Z_OFFSET: -10.03
- GYRO offsets: minimal (<0.1)

**Note:** Calibrate for your specific MPU6050 for best gesture accuracy.

### WiFi Configuration
Update in firmware before uploading:
```cpp
const char* gemini_ssid = "YOUR_SSID";
const char* gemini_password = "YOUR_PASSWORD";
```

### Twilio SMS Configuration
Update in firmware for SOS feature:
```cpp
const char* twilio_account_sid = "ACxxxxx";
const char* twilio_auth_token = "your_token";
const char* twilio_from_number = "+1234567890";
const char* twilio_to_number = "+0987654321";
```

### Serial Monitoring
- **Baud Rate:** 115200
- **Provides:**
  - Boot messages
  - Mode transitions
  - Error diagnostics
  - Button press feedback
  - Braille input display
  - Network status
  - File operations

***

## About the Author

**Mᴀᴀᴛʜᴇs Tʜɪʟᴀᴋ K**  
Bachelor of Engineering – ECE  
Madras Institute of Technology (MIT), Anna University, Chennai  

### Contact & Project Links
- **GitHub:** github.com/MAATHES-THILAK-K
- **Project Repository:** Stellar_Vision_V1
- **Firmware Releases:** Check repository for latest versions

### Interests & Aspirations
- Embedded Systems Design  
- Robotics & Automation  
- Assistive Technology Development  
- Human-Computer Interaction
- Accessibility Engineering

### Development Philosophy
*"Vibe Coding — Leveraging AI as a collaborative tool to accelerate innovation and create meaningful accessibility solutions."*

### Acknowledgments
- **ChronosESP32** library for BLE notifications
- **VoiceRSS** for text-to-speech API
- **OCR.space** for optical character recognition
- **Google Gemini** for AI integration
- **Twilio** for SMS emergency alerts
- Open-source community for Arduino libraries

***

### **Sᴛᴇʟʟᴀʀ Vɪsɪᴏɴ V1 – Making Technology Accessible**  
**Firmware Version:** V3.2  
**Last Updated:** December 2024  
**Developed by:** Mᴀᴀᴛʜᴇs Tʜɪʟᴀᴋ K  

***

## Appendix A: Audio File List

### Required System Audio Files
All files should be in `/TACTI_VISION_WAV/` folder:

| Filename | Purpose | When Played |
|----------|---------|-------------|
| StellarVision.wav | Boot greeting | Device startup |
| PERKINS_MODE.wav | Mode entry | Entering Perkins mode |
| MODES.wav | Mode list | Opening mode options |
| SAVED.wav | Save confirm | File saved successfully |
| SELECTED.wav | Selection confirm | Item selected |
| PREVIOUS.wav | Back action | Going back/canceling |
| ONE_HAND_MODE.wav | Gesture mode | Gesture mode activated |
| SOS.wav | Emergency alert | SOS triggered |
| Deepsleep.wav | Sleep warning | Before entering sleep |
| Alpha_enab.wav | Feature enabled | Alphabet audio ON |
| Alpha_disable.wav | Feature disabled | Alphabet audio OFF |
| Autocorrection.wav | Text correction | Correction applied |
| OCR_Completed.wav | OCR done | B-DRIVE extraction complete |
| File_Upload.wav | Upload success | SD file uploaded |
| converting_TTS.wav | TTS start | Beginning text-to-speech |
| hold_save.wav | Instruction | Note-maker ready |
| hold_query.wav | Instruction | Gemini query ready |
| Sending_Query.wav | Network action | Sending to Gemini |
| response_received.wav | AI response | Gemini reply ready |
| Connecting_wifi.wav | Network status | WiFi connecting |
| wificonnected.wav | Network success | WiFi connected |
| Firmware_check.wav | Update process | Checking for updates |
| firmware_available.wav | Update alert | New version found |
| Downloading.wav | Download status | Firmware downloading |
| update_sucess.wav | Update complete | Update successful |
| latestversion.wav | Version status | Already up to date |
| READY_TO_RECEIVE.wav | Server ready | SD upload server started |
| Audio_Init.wav | Player init | Audio player started |
| playbackstopped.wav | Playback stop | Audio stopped |
| NOTIFY.wav | Mode name | Notify option |
| GEMINI_AI.wav | Mode name | Gemini AI option |
| NOTE_MAKER.wav | Mode name | Note maker option |
| HID_SHORTCUT.wav | Mode name | HID shortcut option |
| B_DRIVE.wav | Mode name | B-Drive option |
| SD_MODE.wav | Mode name | SD card option |
| SD_UPLOAD.wav | Mode name | SD upload option |
| SYSTEM_UPDATE.wav | Mode name | System update option |
| AUDIO_PLAYER.wav | Mode name | Audio player option |
| WIFI.wav | Shortcut name | WiFi shortcut |
| CHROME.wav | Shortcut name | Chrome shortcut |
| GMAIL.wav | Shortcut name | Gmail shortcut |
| CHATGPT.wav | Shortcut name | ChatGPT shortcut |
| PY_COMPILER.wav | Shortcut name | Python compiler shortcut |
| LIBRARY.wav | Shortcut name | Library shortcut |
| num.wav | Mode indicator | Number mode activated |
| caps.wav | Mode indicator | Capital mode activated |

**Recommended Format:** 22kHz, 16-bit, Mono, WAV

***

## Appendix B: Version History

### V3.2 (Current - December 2024)
**Major Features:**
- Enhanced I2C/I2S conflict management
- SOS emergency SMS with 30-second cooldown
- Alphabet audio toggle (UP long press or all 6 dots)
- Text auto-correction (15 common typos)
- SD Upload with folder structure support
- TTS chunking (300 characters with sentence detection)
- Deep sleep safeguards (motor protection)
- OTA update with timeout and confirmation

**Improvements:**
- Stable multi-device I2C (100kHz)
- HID keyboard auto-reconnection
- DigiSpark recovery after audio
- Vibration motor safeguards
- Enhanced error handling

**Bug Fixes:**
- Fixed I2C freeze after I2S audio
- Resolved vibration motor sleep issues
- Corrected SOS false triggers
- Fixed folder upload structure preservation

### V1.0 (Original - November 2024)
**Initial Release:**
- Basic Braille input
- PERKINS mode HID output
- NOTE-MAKER functionality
- SD card navigation
- GEMINI AI integration
- B-DRIVE OCR
- NOTIFY BLE notifications
- Audio feedback system
- Gesture mode
- Deep sleep

***

## Appendix C: Hardware Assembly Notes

### I2C Bus Configuration
**Devices on Bus:**
1. PCF8575 (0x20) - GPIO expander for 14 buttons
2. DigiSpark (0x23) - HID keyboard interface
3. MPU6050 (0x68) - Accelerometer for gestures

**Critical Settings:**
- Bus speed: 100kHz (NOT 400kHz)
- Timeout: 500ms
- Pull-up resistors: 4.7kΩ recommended

### Power Requirements
- **Minimum:** 5V 1A
- **Recommended:** 5V 2A (for stable operation)
- **Peak current:** ~800mA (during WiFi + audio + vibration)

### SD Card Compatibility
- **Format:** FAT32 only
- **Capacity:** Up to 32GB tested
- **Speed Class:** Class 10 or higher recommended
- **Brands tested:** SanDisk, Samsung, Kingston, Amazon Basics

### Audio Amplifier
- **Model:** MAX98357A I2S
- **Gain:** Set to 0.9 in software (adjustable)
- **Speaker:** 4-8Ω, 3W recommended
- **Volume:** Controlled by software gain setting

***

**End of User Manual**

---

*For latest updates, visit the GitHub repository.*  
*For support, check serial output for diagnostic messages.*  
*For emergency assistance, use the SOS feature (1.5s Space hold).*
