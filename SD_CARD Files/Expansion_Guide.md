# Audio Expansion Guide 🎵

## How to Add New Audio Files to Your Device

### Step 1: Create Audio Files with VoiceMaker

1. **Visit**: https://voicemaker.in/
2. **Login/Sign Up** (free account available)
3. **Configure Settings**:
   ```
   Voice: John 2 (M)
   Volume: 20dB
   Format: WAV
   Sample Rate: 44100Hz
   ```

4. **Enter Text** and click "Generate"
5. **Download** the audio file after processing

### Step 2: File Naming Convention

| Purpose | Suggested Filename | Text Content Example |
|---------|-------------------|---------------------|
| Mode Entry | `MODE_ENTRY.wav` | "Entering Note Maker Mode" |
| Success | `SUCCESS.wav` | "Operation completed successfully" |
| Error | `ERROR.wav` | "Error occurred, please try again" |
| Saved | `SAVED.wav` | "File saved successfully" |
| Custom | `CUSTOM_ACTION.wav` | Your custom message |

### Step 3: Upload to Device

1. **Enter SD Upload Mode** on your device
2. **Access Web Interface**: `http://[device-ip]:8080`
3. **Upload Files** to `/AudioFiles/` folder
4. **Verify** files appear in SD Navigation mode

### Step 4: Configure in Code

Add new audio file references in your code:

```cpp
// Add to your audio file definitions
void playModeAudio(AppMode mode) {
  switch(mode) {
    case NOTE_MAKER_MODE:
      playWAV("/AudioFiles/MODE_ENTRY.wav");
      break;
    // Add your new modes here
    case CUSTOM_MODE:
      playWAV("/AudioFiles/CUSTOM_ACTION.wav");
      break;
  }
}

// Or trigger specific actions
void playSuccessSound() {
  playWAV("/AudioFiles/SUCCESS.wav");
}
```

### Step 5: Available Audio Slots

You can create audio files for:

#### **System Events**
- `BOOTUP.wav` - Device startup sound
- `SHUTDOWN.wav` - Power off sound
- `LOW_BATTERY.wav` - Battery warning
- `CONNECTED.wav` - WiFi/BLE connected

#### **Mode Transitions**
- `ENTER_[MODE_NAME].wav` - Each mode entry
- `EXIT_[MODE_NAME].wav` - Mode exit confirmation

#### **User Actions**
- `SAVED.wav` - File save confirmation
- `DELETED.wav` - Delete confirmation
- `UPLOADED.wav` - Upload complete
- `ERROR.wav` - Operation failed

#### **Navigation**
- `SCROLL_UP.wav` - Moving up in lists
- `SCROLL_DOWN.wav` - Moving down
- `SELECTED.wav` - Item selected

### Step 6: VoiceMaker Tips for Best Quality

1. **Text Formatting**:
   ```
   Good: "File saved successfully"
   Avoid: "FILE SAVED SUCCESSFULLY!" (too aggressive)
   ```

2. **Pacing**: Add commas for natural pauses
   ```
   "Operation completed, successfully"
   ```

3. **Volume**: Stick to 20dB for consistent levels

4. **Testing**: Generate short samples first to test voice clarity

### Step 7: File Management

**Recommended Folder Structure**:
```
/AudioFiles/
├── system/
│   ├── BOOTUP.wav
│   ├── SHUTDOWN.wav
│   └── LOW_BATTERY.wav
├── modes/
│   ├── NOTE_MAKER.wav
│   ├── GEMINI_AI.wav
│   └── B_DRIVE.wav
├── actions/
│   ├── SUCCESS.wav
│   ├── ERROR.wav
│   └── SAVED.wav
└── navigation/
    ├── SCROLL_UP.wav
    ├── SCROLL_DOWN.wav
    └── SELECTED.wav
```

### Step 8: Memory Considerations

- **Average file size**: ~50-100KB per 3-second audio
- **SD Card**: Use Class 10 for smooth playback
- **Total audio library**: Keep under 50MB for quick loading

### Example: Adding a New Mode Sound

1. **Create**: "Entering Weather Mode" on VoiceMaker
2. **Save as**: `WEATHER_MODE.wav`
3. **Upload** to `/AudioFiles/modes/WEATHER_MODE.wav`
4. **Add to code**:
   ```cpp
   case WEATHER_MODE:
     playWAV("/AudioFiles/modes/WEATHER_MODE.wav");
     break;
   ```

Now you can expand your device's audio feedback system with custom, high-quality voice prompts! 🎉

---
**NOTE : I AM USING 32 GB FAT32 SUPPORTED SD CARD FROM AMAZON BASICS**
