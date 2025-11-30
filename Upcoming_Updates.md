## 🚀 Hardware Upgrades

### **Bookmark System for Audio**
- **Inspired by Visually Impared user feedback**:
  - Voice-commanded bookmarks: "Bookmark this"
  - Quick navigation through long content
  - Chapter markers for audiobooks
  - Resume from last position
### **ESP IDF Migration**
  - To Make more efficient firmware
  - To deploy as a Product that is affordable for everyone

 Below is a **much simpler, cleaner, minimal version** while keeping a professional tone.

### **Rubber-Dome Key Switch Upgrade**

The next hardware revision will use **rubber-dome (TV-remote style) key switches** to improve comfort and typing accuracy,and also add the female usb port for HID Emulation (As of now fixed USB cable is there).

#### **Benefits**

* **Soft, quiet keys** for better long-term comfort.
* **Reliable multi-press performance**, ideal for Braille chords.
* **Consistent tactile feedback** to help users confirm each input.
* **Long lifespan** (1–5 million presses).
* **Slim design**, suitable for compact assistive devices.

  ---

### 1. **mmWave Sensor for Person Detection**
- **Sensor**: RCWL-9196 or LD2410 mmWave radar
- **Purpose**: Detect human presence without cameras
- **Features**:
  - Presence detection for auto-wake/sleep
  - Gesture recognition enhancement
  - Privacy-preserving (no visual data)
- **Integration**: Low-power mode with interrupt wakeup

### 2. **Aux Jack for Wired Headset**
- **Connector**: 3.5mm TRRS audio jack
- **Purpose**: Private audio output
- **Features**:
  - Auto-switch between speaker/headphone
  - Noise isolation for better clarity
  - Accessibility for hearing aids

### 3. **Refreshable Braille Display**
- **Technology**: Based on Vijay Varada's open-source design https://hackaday.io/project/10849-refreshable-braille-display
- **Specs**:
  - 20-cell refreshable braille
  - Low power consumption
- **Integration**:
  - Real-time text-to-braille conversion
  - Tactile feedback for navigation

### 4. **ESP32-S3 Migration**
- **Processor**: ESP32-S3 (Dual-core Xtensa LX7)
- **Advantages**:
  -Eliminate Digispark (Full USB HID)  
  - **Core 0**: Handle I2S audio, BLE, WiFi
  - **Core 1**: Handle I2C sensors, GPIO, braille display
  - Eliminate I2C/I2S conflicts
  - Built-in USB OTG for direct HID
  - More GPIO (no need for PCF8575 expander)

### 5. **Power System & Battery**
- **Target**: 12 hours continuous operation
- **Battery**: 5000mAh LiPo @ 3.7V
- **Power Management**:
  - **Current Calculation**:
    ```
    ESP32-C6 Wi-Fi/BLE TX peak = 316 mA
    microSD write peak = 100 mA
    Vibration motor startup/peak = 150 mA
    Digispark (ATtiny85) active = 30 mA
    MPU sensor = 5 mA
    PCF GPIO expander + switches = 5 mA
    Audio amplifier + 3W speaker = 1000 mA (peak)
    mmWave sensor = 45 mA
    Braille display = 200 mA (peak)
    ──────────────────────────────────────
    TOTAL PEAK = 1851 mA
    AVERAGE (estimated) = 450-600 mA
    ```
  - **Expected Runtime**: 5000mAh / 500mA ≈ 10 hours
  - **Charging**: USB-C PD with battery management IC

### 6. **PCB & Mechanical Improvements**
- **Custom PCB**:
  - Smooth tactile keys
  - Better key matrix layout
  - Integrated audio amplifier
  - Proper power distribution
- **Enclosure**:
  - Ergonomic 3D printed case

## 💾 Software & Storage Upgrades

### 7. **SPIFFS for Website & Resources**
- **Replace SD card for static content**:
  - Web server assets (HTML, CSS, JS)
  - Expanded text correction dictionary
  - Audio prompts and system sounds
  - Configuration files
- **Benefits**:
  - Faster access times
  - More reliable than SD card
  - Lower power consumption

### 8. **Low-Power Tilt Detection**
- **Alternative to MPU6050**:
  - MMA8452Q (low-power accelerometer)
  - LIS2DH (ultra-low-power option)
  - Interrupt-driven wakeup
- **Power Savings**: 80% reduction in motion sensing power


## 🎯 Key Benefits

1. **No I2C/I2S Conflicts** → Better reliability
2. **12-Hour Battery** → Full-day usage
3. **Private Audio** → Headphone support
4. **Tactile Output** → Braille display
5. **Person Detection** → Context awareness
6. **Better Input** → Smooth key switches
7. **Faster Access** → SPIFFS storage
8. **Smarter Correction** → Expanded dictionary (Expecially useful in Notemaker and perkins Mode)
