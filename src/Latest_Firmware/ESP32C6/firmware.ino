#include <Arduino.h>
#include <map>
#include <string>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <vector>
#include <algorithm>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ChronosESP32.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <AudioGeneratorWAV.h>
#include <AudioFileSourceSD.h>
#include <AudioOutputI2S.h>
#include <Update.h>
#include <esp_sleep.h>

// MPU6050 setup
Adafruit_MPU6050 mpu;

// ================== OTA UPDATE CONFIG ==================
#define CURRENT_FIRMWARE_VERSION "V3.2"
const char* latestInfoURL = "https://raw.githubusercontent.com/MAATHES-THILAK-K/Stellar_Vision_V1/main/Firmware/latest.json";

// Calibration Offsets - Calibrate them based on your MPU6050
#define ACCEL_X_OFFSET 0.88
#define ACCEL_Y_OFFSET -0.20
#define ACCEL_Z_OFFSET -10.03
#define GYRO_X_OFFSET -0.04
#define GYRO_Y_OFFSET 0.00
#define GYRO_Z_OFFSET 0.00

// ================== PCF8575 GPIO Expander ==================
#define PCF8575_ADDR 0x20
#define PCF8575_SDA 0
#define PCF8575_SCL 1

#define MPU_ADDR 0x68
#define DEEP_SLEEP_TIMEOUT 30000

// HID Keyboard via DigiSpark
#define DIGISPARK_ADDR 0x23
#define HID_CMD_PRINT_CHAR 0x01
#define HID_CMD_BACKSPACE 0x02
#define HID_CMD_ENTER 0x03
#define HID_CMD_SPACE 0x04
#define HID_CMD_CTRL 0x05
#define HID_CMD_SHIFT_ENTER 0x06
#define HID_CMD_CTRL_PRESS 0x07
#define HID_CMD_CTRL_RELEASE 0x08

// ================== HID SHORTCUT COMMANDS ==================
#define HID_CMD_OPEN_WIFI 0x10
#define HID_CMD_OPEN_CHROME 0x11
#define HID_CMD_OPEN_GMAIL 0x12
#define HID_CMD_OPEN_CHATGPT 0x13
#define HID_CMD_OPEN_PYCOMPILER 0x14
#define HID_CMD_OPEN_LIBRARY 0x15

int selectedShortcutIndex = 0;
const char* shortcutNames[] = {
  "WIFI",
  "CHROME",
  "GMAIL",
  "CHATGPT",
  "PY COMPILER",
  "LIBRARY"
};
const int NUM_SHORTCUTS = sizeof(shortcutNames) / sizeof(shortcutNames[0]);

bool hidKeyboardEnabled = false;
bool hidInitialized = false;

const char* twilio_account_sid = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
const char* twilio_auth_token = "xxxxxxxxxxxxxxxxxxxxxxxxxxxx";
const char* twilio_from_number = "xxxxxxxxxxx";
const char* twilio_to_number = "xxxxxxxxxxxxx";

bool sosTriggered = false;
unsigned long lastSOSTime = 0;
bool sosInitialized = false;
const unsigned long SOS_COOLDOWN = 30000;  // 30 seconds cooldown to prevent spam

const int pcfBraillePins[] = { 0, 1, 2, 3, 4, 5 };  // Pins 0-5: Braille dots 1-6
const int PCF_CTRL_PIN = 6;                         // Pin 6: Ctrl modifier
const int PCF_BACKSPACE_PIN = 7;                    // Pin 7: Backspace
const int PCF_SPACE_LEFT_PIN = 8;                   // Pin 8: Space (left)
const int PCF_SPACE_RIGHT_PIN = 9;                  // Pin 9: Space (right)
const int PCF_PREV_PIN = 10;                        // Pin 10: Previous
const int PCF_UP_PIN = 14;                          // Pin 11: Up navigation
const int PCF_SELECT_PIN = 12;                      // Pin 12: Select/Enter
const int PCF_DOWN_PIN = 13;

// ================== I2S / MAX98357A Pins ==================
#define I2S_BCLK 12
#define I2S_LRC 13
#define I2S_DIN 15

// Tilt detection thresholds
#define TILT_THRESHOLD 3.0
unsigned long lastTiltTime = 0;
const unsigned long TILT_DEBOUNCE = 450;

#define SD_CS_PIN 5
#define SD_SCK_PIN 23
#define SD_MISO_PIN 21
#define SD_MOSI_PIN 22
#define VIB_PIN 2

const int NUM_BRAILLE_PINS = 6;

bool currentBrailleStates[NUM_BRAILLE_PINS];
bool lastBrailleStates[NUM_BRAILLE_PINS];

//Individual state variables for all 14 keys
bool currentCtrlState, lastCtrlState;
bool currentBackspaceState, lastBackspaceState;
bool currentSpaceLeftState, lastSpaceLeftState;
bool currentSpaceRightState, lastSpaceRightState;
bool currentPrevState, lastPrevState;
bool currentUpState, lastUpState;
bool currentSelectState, lastSelectState;
bool currentDownState, lastDownState;
bool alphabetAudioEnabled = false;

unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;
unsigned long lastBrailleCharTime = 0;
const unsigned long BRAILLE_CHAR_DELAY = 150;
// SOS trigger timing - increased from 400ms to 3 seconds
const unsigned long SOS_LONG_PRESS_DURATION = 1500;  // 1.5 seconds for SOS

//Button press timing variables
unsigned long selectPressStartTime = 0;
unsigned long lastSelectTapTime = 0;
unsigned long prevPressStartTime = 0;
const unsigned long LONG_PRESS_SELECT_DURATION = 1000;
const unsigned long LONG_PRESS_PREV_DURATION = 1000;

//Combo detection for Space+Ctrl and Space+Backspace
unsigned long spaceCtrlComboStartTime = 0;
unsigned long spaceBackspaceComboStartTime = 0;
bool spaceCtrlPressed = false;
bool spaceBackspacePressed = false;

//Combo detection for Ctrl+Backspace (6+7)
unsigned long ctrlBackspaceComboStartTime = 0;
bool ctrlBackspacePressed = false;
const unsigned long COMBO_HOLD_DURATION = 400;

unsigned long suppressSingleUntil = 0;
const unsigned long SUPPRESS_SINGLE_MS = 450;

// OTA Update state
bool otaUpdateAvailable = false;
bool otaWaitingForConfirm = false;
String otaNewVersion = "";
String otaFirmwareURL = "";
unsigned long otaConfirmTimeout = 0;
const unsigned long OTA_CONFIRM_TIMEOUT = 15000;  // 15 seconds

// Audio objects
AudioGeneratorWAV* wav = nullptr;
AudioFileSourceSD* audioFile = nullptr;
AudioOutputI2S* audioOut = nullptr;

SPIClass sd_spi(HSPI);
ChronosESP32 watch;

bool sdCardAvailable = false;

// B-DRIVE web server (Braille-Drive)
WebServer bdriveServer(80);
String bdriveExtractedText = "";
bool bdriveTextReady = false;
bool bdriveWaitingForSave = false;
bool bdriveNaming = false;
String bdriveFileName = "";

// SD Upload server state
WebServer sdUploadServer(8080);
bool sdUploadServerActive = false;

unsigned long lastActivityTime = 0;
bool deepSleepEnabled = false;
// Modes where deep sleep is DISABLED
bool initHIDKeyboard() {
  if (hidInitialized) return true;

  //Serial.println("Checking for DigiSpark...");
  Wire.beginTransmission(DIGISPARK_ADDR);
  uint8_t error = Wire.endTransmission();

  if (error == 0) {
    hidInitialized = true;
    Serial.println("✅ DigiSpark found and initialized");
    return true;
  } else {
    Serial.println("⚠️  DigiSpark not found - HID disabled");
    return false;
  }
}

// Send command to DigiSpark HID keyboard
bool sendHIDCommand(uint8_t cmd, uint8_t data = 0) {
  if (!hidKeyboardEnabled || !hidInitialized) return false;

  // ***Add small delay before I2C communication ***
  delay(5);  // Prevent I2C bus conflicts

  Wire.beginTransmission(DIGISPARK_ADDR);
  Wire.write(cmd);
  Wire.write(data);
  uint8_t error = Wire.endTransmission();

  if (error != 0) {
    Serial.printf("[HID] ❌ Send failed (error %d)\n", error);

    // *** Try recovery on first error ***
    if (error == 2 || error == 5) {  // NACK or timeout
      delay(10);
      Wire.beginTransmission(DIGISPARK_ADDR);
      Wire.write(cmd);
      Wire.write(data);
      error = Wire.endTransmission();

      if (error == 0) {
        //Serial.println("✅ Retry successful");
        return true;
      }
    }

    return false;
  }

  delay(5);
  return true;
}

// Send a character to HID keyboard
void hidPrintChar(char c) {
  if (sendHIDCommand(HID_CMD_PRINT_CHAR, (uint8_t)c)) {
    //Serial.print("[HID→]");
    //Serial.print(c);
  }
}

// Send Ctrl key press
void hidCtrlPress() {
  if (sendHIDCommand(HID_CMD_CTRL_PRESS)) {
    //Serial.print("[HID→CTRL_PRESS]");
  }
}

// Send Ctrl key release
void hidCtrlRelease() {
  if (sendHIDCommand(HID_CMD_CTRL_RELEASE)) {
    //Serial.print("[HID→CTRL_RELEASE]");
  }
}

// Send Shift+Enter
void hidShiftEnter() {
  if (sendHIDCommand(HID_CMD_SHIFT_ENTER)) {
    //Serial.print("[HID→SHIFT+ENTER]");
  }
}

// Send backspace to HID keyboard
void hidBackspace() {
  if (sendHIDCommand(HID_CMD_BACKSPACE)) {
    //Serial.print("[HID→BACKSPACE]");
  }
}

// Send enter to HID keyboard
void hidEnter() {
  if (sendHIDCommand(HID_CMD_ENTER)) {
    //Serial.print("[HID→ENTER]");
  }
}

// Send space to HID keyboard
void hidSpace() {
  if (sendHIDCommand(HID_CMD_SPACE)) {
    //Serial.print("[HID→SPACE]");
  }
}

// Enable/disable HID keyboard for current mode
void setHIDKeyboardMode(bool enable) {
  if (enable && !hidInitialized) {
    initHIDKeyboard();
  }
  hidKeyboardEnabled = enable && hidInitialized;

  if (enable && hidInitialized) {
    //Serial.println("Keyboard output ENABLED");
  } else if (enable) {
    //Serial.println("Keyboard output DISABLED (DigiSpark not found)");
  } else {
    //Serial.println("Keyboard output DISABLED");
  }
}

// OCR.space API credentials
String ocrApiKey = "xxxxxxxxxxxxxxxx";
const char* ocrHost = "api.ocr.space";
const int ocrPort = 443;

// ===== VoiceRSS TTS API =====
#define VOICERSS_API_KEY "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define VOICERSS_URL "http://api.voicerss.org/"

// Notification ring buffer
#define NOTIF_RING_SIZE 8
struct NotifFixed {
  char time[32];
  char app[64];
  char title[128];
  char message[1024];
};

// GEMINI AI storage
String geminiResponseText = "";
bool geminiWaitingForSave = false;
bool geminiNaming = false;
String geminiFileName = "";

volatile uint8_t notif_head = 0;
volatile uint8_t notif_tail = 0;
NotifFixed notif_ring[NOTIF_RING_SIZE];

static inline bool notif_ring_full() {
  return ((notif_head + 1) % NOTIF_RING_SIZE) == notif_tail;
}

static inline bool notif_ring_empty() {
  return notif_head == notif_tail;
}

enum AppMode {
  BOOT_MODE,
  NORMAL_MODE,
  PERKINS_MODE,
  MODE_OPTIONS,
  NOTE_MAKER_MODE,
  SD_NAVIGATION_MODE,
  NOTIFY_MODE_PLACEHOLDER,
  GEMINI_AI_MODE_PLACEHOLDER,
  HID_SHORTCUT_MODE_PLACEHOLDER,
  B_DRIVE_MODE,
  GESTURE_MODE,
  SD_UPLOAD_MODE,
  SYSTEM_UPDATE_MODE,
  AUDIO_PLAYER_MODE
};

AppMode currentAppMode = BOOT_MODE;
AppMode previousAppMode = BOOT_MODE;

std::string currentWord = "";
std::string lastSavedText = "";
bool isNumberMode = false;
bool isCapitalMode = false;

std::map<int, char> brailleMap;
std::map<int, char> numberMap;

const int NUMERIC_INDICATOR_PATTERN = 60;
const int CAPITAL_INDICATOR_PATTERN = 32;

const char* modeNames[] = {
  "NOTIFY",
  "GEMINI AI",
  "NOTE-MAKER",
  "HID SHORTCUT",
  "B-DRIVE",
  "SD CARD",
  "SD UPLOAD",
  "SYSTEM UPDATE",
  "AUDIO PLAYER"
};

AppMode modeOptionValues[] = {
  MODE_OPTIONS,
  GEMINI_AI_MODE_PLACEHOLDER,
  NOTE_MAKER_MODE,
  HID_SHORTCUT_MODE_PLACEHOLDER,
  B_DRIVE_MODE,
  SD_NAVIGATION_MODE,
  SD_UPLOAD_MODE,
  SYSTEM_UPDATE_MODE,
  AUDIO_PLAYER_MODE
};

bool isDeepSleepExemptMode() {
  // Return TRUE for modes that should NEVER sleep (stay awake)
  return (currentAppMode == B_DRIVE_MODE || currentAppMode == SD_UPLOAD_MODE || currentAppMode == SD_NAVIGATION_MODE || currentAppMode == SYSTEM_UPDATE_MODE || currentAppMode == AUDIO_PLAYER_MODE || currentAppMode == MODE_OPTIONS || currentAppMode == BOOT_MODE || currentAppMode == NORMAL_MODE);
}


const int NUM_MODE_OPTIONS = sizeof(modeNames) / sizeof(modeNames[0]);
int selectedModeIndex = 0;

std::vector<String> sdFiles;
int currentFileIndex = 0;
String newNoteFileName;
bool gestureModeActive = false;
unsigned long lastBothComboTime = 0;
const unsigned long BOTH_COMBO_COOLDOWN = 800;
bool isNaming = false;
//finished
const char* gemini_ssid = "KMT's MOBILE";
const char* gemini_password = "asdfghjkl";
const char* gemini_api_key = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
const char* gemini_api_url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent";

bool notifyActive = false;
unsigned long notifyLastActivity = 0;
bool bleInitialized = false;

// Audio Player Mode state
std::vector<String> audioFiles;
int currentAudioIndex = 0;
bool audioPlaying = false;
bool audioPlayerActive = false;

// Function declarations
void vibe(float seconds);
void playWAV(const char* filename);
void populateBrailleMaps();
int readBraillePattern();
void processBrailleInput(int pattern);
void enterMode(AppMode newMode);
void listSdFiles(const char* dirname, std::vector<String>& files);
void enterNoteMakerNaming();
void saveNoteToFile(const String& filename, const String& content);
String generateUniqueNoteName();
String generateUniqueBdriveName();
String normalizeSdPath(const String& pathIn);
bool ensureWiFiConnected();
String queryGemini(const String& userQuery);
void chronosConnectionCallback(bool state);
void chronosNotificationCallback(Notification notification);
void chronosRingerCallback(String caller, bool state);
void notifyMobileConnected(const char* deviceName);
void notifyMobileDisconnected();
void startNotificationBLE();
void stopNotificationBLE();
void appendNotificationShort(const NotifFixed& n);
void processNotificationRingBuffer();
void startBdriveServer();
void stopBdriveServer();
void handleBdriveRoot();
void saveBdriveFile(const String& filename, const String& content);
String sendToOCRFromFile(const char* filepath);
void handleImageUpload();
void handleTextReceive();
// SD Upload Mode
void startSDUploadServer();
void stopSDUploadServer();
void handleSDUploadRoot();
void handleFileUploadToSD();
String getSDFileList();
void handleDeleteFile();
void handleDownloadFile();
void cleanupAudio();
void reinitI2C();
void checkKeyCombinations();

// ==== UPDATE ACTIVITY TIMER ====
void updateActivityTimer() {
  lastActivityTime = millis();
}

uint16_t readPCF8575() {
  Wire.beginTransmission(PCF8575_ADDR);
  if (Wire.endTransmission() != 0) {
    //Serial.println("[PCF8575] Device not responding!");
    return 0xFFFF;
  }

  Wire.requestFrom(PCF8575_ADDR, (uint8_t)2);
  if (Wire.available() < 2) {
    //Serial.println("[PCF8575] Read error!");
    return 0xFFFF;
  }

  uint8_t lsb = Wire.read();
  uint8_t msb = Wire.read();
  uint16_t val = ((uint16_t)msb << 8) | lsb;
  return val;
}

void writePCF8575(uint16_t value) {
  Wire.beginTransmission(PCF8575_ADDR);
  Wire.write(value & 0xFF);
  Wire.write((value >> 8) & 0xFF);
  if (Wire.endTransmission() != 0) {
    //Serial.println("[PCF8575] Write failed!");
  }
}

std::map<String, String> correctionMap;

void populateCorrectionMap() {
  correctionMap["teh"] = "the";
  correctionMap["pleese"] = "please";
  correctionMap["tnanks"] = "thanks";
  correctionMap["heelp"] = "help";
  correctionMap["cal"] = "call";
  correctionMap["fone"] = "phone";
  correctionMap["mesage"] = "message";
  correctionMap["reed"] = "read";
  correctionMap["adio"] = "audio";
  correctionMap["braile"] = "braille";
  correctionMap["scren"] = "screen";
  correctionMap["voise"] = "voice";
  correctionMap["setings"] = "settings";
  correctionMap["batery"] = "battery";
  correctionMap["emergncy"] = "emergency";
}


void updateSwitchStatesFromPCF() {
  static uint16_t lastPcfState = 0xFFFF;  // Initialize to all HIGH (no buttons pressed)

  uint16_t pcfState = readPCF8575();

  // Only update activity timer if the state changed
  if (pcfState != lastPcfState) {
    updateActivityTimer();
    lastPcfState = pcfState;
  }

  // Update braille pin states (inverted logic: LOW = pressed)
  for (int i = 0; i < NUM_BRAILLE_PINS; i++) {
    currentBrailleStates[i] = bitRead(pcfState, pcfBraillePins[i]) ? HIGH : LOW;
  }

  // Update all other key states
  currentCtrlState = bitRead(pcfState, PCF_CTRL_PIN) ? HIGH : LOW;
  currentBackspaceState = bitRead(pcfState, PCF_BACKSPACE_PIN) ? HIGH : LOW;
  currentSpaceLeftState = bitRead(pcfState, PCF_SPACE_LEFT_PIN) ? HIGH : LOW;
  currentSpaceRightState = bitRead(pcfState, PCF_SPACE_RIGHT_PIN) ? HIGH : LOW;
  currentPrevState = bitRead(pcfState, PCF_PREV_PIN) ? HIGH : LOW;
  currentUpState = bitRead(pcfState, PCF_UP_PIN) ? HIGH : LOW;
  currentSelectState = bitRead(pcfState, PCF_SELECT_PIN) ? HIGH : LOW;
  currentDownState = bitRead(pcfState, PCF_DOWN_PIN) ? HIGH : LOW;
}


void notifyMobileConnected(const char* deviceName) {
  //Serial.print("CHRONOS MOBILE CONNECTED: ");
  //Serial.println(deviceName);
  vibe(0.12);
  notifyLastActivity = millis();
}

void notifyMobileDisconnected() {
  Serial.println("CHRONOS MOBILE DISCONNECTED");
  vibe(0.08);
}

void startNotificationBLE() {
  if (!notifyActive) {
    notifyActive = true;
    notifyLastActivity = millis();
    //Serial.println("[NOTIFY] listener STARTED");

    if (!bleInitialized) {
      watch.begin();
      bleInitialized = true;
      Serial.println("✅ [BLE] ChronosESP32 initialized");
    }
    vibe(0.2);
  }
}

void stopNotificationBLE() {
  if (notifyActive) {
    notifyActive = false;
    Serial.println("[NOTIFY] listener STOPPED");
    vibe(0.12);
  }
}

void vibe(float seconds) {
  digitalWrite(VIB_PIN, HIGH);
  delay(static_cast<unsigned long>(seconds * 1000));
  digitalWrite(VIB_PIN, LOW);
}

// ==== Safely disable I2C devices before I2S audio ==== -> To avoid I2C and I2S conflicts
void prepareForAudio() {
  //Serial.println("Preparing for audio playback...");

  // Temporarily disable HID to prevent I2C conflicts during I2S
  if (hidKeyboardEnabled) {
    //Serial.println("[HID] Temporarily pausing for audio");
    hidKeyboardEnabled = false;
  }

  delay(50);  // Allow any pending I2C transactions to complete
}

void playWAV(const char* filename) {
  if (!sdCardAvailable) {
    //Serial.print("[WAV: ");
    //Serial.print(filename);
    //Serial.println(" - SD not available]");
    vibe(0.15);
    return;
  }

  //Serial.print("[Playing WAV: ");
  //Serial.print(filename);
  //Serial.println("]");

  // *** Prepare I2C devices for audio playback ***
  bool wasHIDEnabled = hidKeyboardEnabled;
  prepareForAudio();

  // Clean up previous objects if they exist
  if (wav) {
    if (wav->isRunning()) {
      wav->stop();
    }
    delete wav;
    wav = nullptr;
  }
  if (audioFile) {
    delete audioFile;
    audioFile = nullptr;
  }

  // Create audio source and generator
  audioFile = new AudioFileSourceSD(filename);
  wav = new AudioGeneratorWAV();

  if (!wav->begin(audioFile, audioOut)) {
    //Serial.println("[ERROR: Cannot play WAV!]");
    vibe(0.15);

    // *** Clean up and restore I2C even on error ***
    cleanupAudio();
    reinitI2C();

    // Restore HID state
    if (wasHIDEnabled && hidInitialized) {
      hidKeyboardEnabled = true;
    }
    return;
  }

  // Play the audio file (blocking)
  while (wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      break;
    }
  }

  //Serial.println("[Playback finished]");

  // *** Clean up I2S and reinit I2C after playback ***
  cleanupAudio();
  reinitI2C();

  // *** Restore HID keyboard state ***
  if (wasHIDEnabled && hidInitialized) {
    hidKeyboardEnabled = true;
    //Serial.println("Keyboard output re-enabled after audio");
  }
}

String generateUniqueGeminiName() {
  if (!sdCardAvailable) return String("GEMINI1.txt");

  for (int i = 1; i < 1000; i++) {
    String filename = String("GEMINI") + String(i) + String(".txt");
    if (!SD.exists(filename.c_str()) && !SD.exists((String("/") + filename).c_str())) {
      return filename;
    }
  }
  return String("GEMINI_unknown.txt");
}

void chronosConnectionCallback(bool state) {
  if (state) {
    if (notifyActive) {
      //Serial.println("[CHRONOS] Remote device connected (NOTIFY ON).");
      Serial.println("NOTIFY is ON");
      //notifyMobileConnected("MOBILE");
    } else {
      //Serial.println("[CHRONOS] Remote device connected but NOTIFY is OFF — ignoring.");
      //Serial.println("NOTIFY is OFF");
    }
  } else {
    //Serial.println("[CHRONOS] disconnected.");
    notifyMobileDisconnected();
  }
}

void chronosNotificationCallback(Notification notification) {
  if (!notifyActive) {  //when NOTIFY IS OFF
    //Serial.print("[NOTIFY CB] Notification received from '");
    //Serial.print(notification.app);
    //Serial.println("' but NOTIFY is OFF. Ignoring.");
    return;
  }

  Serial.println("Received notification!");
  //Serial.print(" APP: ");
  //Serial.println(notification.app);
  //Serial.print(" TITLE len: ");
  //Serial.println(notification.title.length());
  //Serial.print(" MSG len: ");
  //Serial.println(notification.message.length());

  noInterrupts();
  if (!notif_ring_full()) {
    NotifFixed& dest = notif_ring[notif_head];
    memset(&dest, 0, sizeof(dest));

    strncpy(dest.time, notification.time.c_str(), sizeof(dest.time) - 1);
    dest.time[sizeof(dest.time) - 1] = '\0';

    strncpy(dest.app, notification.app.c_str(), sizeof(dest.app) - 1);
    dest.app[sizeof(dest.app) - 1] = '\0';

    strncpy(dest.title, notification.title.c_str(), sizeof(dest.title) - 1);
    dest.title[sizeof(dest.title) - 1] = '\0';

    strncpy(dest.message, notification.message.c_str(), sizeof(dest.message) - 1);
    dest.message[sizeof(dest.message) - 1] = '\0';

    notif_head = (notif_head + 1) % NOTIF_RING_SIZE;
  } else {
    //Serial.println("[NOTIFY CB] Ring buffer full, dropping notification.");
  }
  interrupts();
}

void appendNotificationShort(const NotifFixed& n) {
  if (!sdCardAvailable) {
    //Serial.println("[NOTIFY] SD card not available, cannot save notification");
    return;
  }

  const char* notifyPath = "/NOTIFY.txt";
  File f = SD.open(notifyPath, FILE_APPEND);
  if (!f) {
    //Serial.print("[NOTIFY] Failed to open ");
    //Serial.println(notifyPath);
    return;
  }

  char buffer[2048];
  snprintf(buffer, sizeof(buffer),
           "App: %s %s\nMsg: %s %s\n---------------------\n",
           n.app, n.time, n.title, n.message);

  size_t written = f.print(buffer);
  Serial.println(buffer);
  f.flush();
  f.close();

  if (written > 0) {
    //Serial.print("[NOTIFY] Successfully wrote ");
    //Serial.print(written);
    //Serial.print(" bytes to ");
    //Serial.println(notifyPath);
  } else {
    //Serial.println("[NOTIFY] Write failed - 0 bytes written!");
  }
}

void chronosRingerCallback(String caller, bool state) {
  //Serial.print("[CALL] ");
  //Serial.print(caller);
  //Serial.println(state ? " is CALLING!" : " call ended.");
  vibe(0.4);
}

void processNotificationRingBuffer() {
  while (!notif_ring_empty()) {
    noInterrupts();
    uint8_t idx = notif_tail;
    NotifFixed localCopy;
    memcpy(&localCopy, &notif_ring[idx], sizeof(NotifFixed));
    notif_tail = (notif_tail + 1) % NOTIF_RING_SIZE;
    interrupts();

    appendNotificationShort(localCopy);
  }
}

void handleBdriveRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TACTI-WAVE B-DRIVE | Text Extraction Portal</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <script src="https://cdnjs.cloudflare.com/ajax/libs/pdf.js/2.16.105/pdf.min.js"></script>
    <style>
        :root {
            --primary: #4361ee;
            --primary-dark: #3a56d4;
            --secondary: #7209b7;
            --accent: #f72585;
            --light: #f8f9fa;
            --dark: #212529;
            --success: #4cc9f0;
            --warning: #f8961e;
            --danger: #e63946;
            --gray: #6c757d;
            --border-radius: 12px;
            --box-shadow: 0 10px 30px rgba(0, 0, 0, 0.08);
            --transition: all 0.3s ease;
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        }

        body {
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
            min-height: 100vh;
            padding: 20px;
            color: var(--dark);
        }

        .container {
            max-width: 800px;
            margin: 0 auto;
        }

        .header {
            text-align: center;
            margin-bottom: 40px;
            padding: 20px;
        }

        .logo {
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 15px;
            margin-bottom: 15px;
        }

        .logo-icon {
            font-size: 2.5rem;
            color: var(--primary);
        }

        h1 {
            font-size: 2.5rem;
            background: linear-gradient(to right, var(--primary), var(--secondary));
            -webkit-background-clip: text;
            background-clip: text;
            color: transparent;
            font-weight: 700;
            margin-bottom: 10px;
        }

        .subtitle {
            font-size: 1.1rem;
            color: var(--gray);
            max-width: 600px;
            margin: 0 auto;
            line-height: 1.6;
        }

        .card {
            background: white;
            border-radius: var(--border-radius);
            box-shadow: var(--box-shadow);
            padding: 30px;
            margin-bottom: 30px;
            transition: var(--transition);
        }

        .card:hover {
            transform: translateY(-5px);
            box-shadow: 0 15px 35px rgba(0, 0, 0, 0.1);
        }

        .card-title {
            font-size: 1.5rem;
            margin-bottom: 20px;
            color: var(--primary);
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .card-title i {
            font-size: 1.3rem;
        }

        .upload-area {
            border: 2px dashed #c3cfe2;
            border-radius: var(--border-radius);
            padding: 40px 20px;
            text-align: center;
            cursor: pointer;
            transition: var(--transition);
            margin-bottom: 20px;
            background: #f8f9fe;
        }

        .upload-area:hover {
            border-color: var(--primary);
            background: #f0f3ff;
        }

        .upload-area i {
            font-size: 3rem;
            color: var(--primary);
            margin-bottom: 15px;
        }

        .upload-text {
            font-size: 1.2rem;
            margin-bottom: 10px;
            color: var(--dark);
        }

        .upload-subtext {
            color: var(--gray);
            margin-bottom: 20px;
        }

        .file-input {
            display: none;
        }

        .btn {
            display: inline-block;
            background: var(--primary);
            color: white;
            border: none;
            padding: 14px 28px;
            border-radius: 50px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: var(--transition);
            text-align: center;
            box-shadow: 0 4px 15px rgba(67, 97, 238, 0.3);
        }

        .btn:hover {
            background: var(--primary-dark);
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(67, 97, 238, 0.4);
        }

        .btn i {
            margin-right: 8px;
        }

        .btn-block {
            display: block;
            width: 100%;
        }

        .btn-success {
            background: var(--success);
            box-shadow: 0 4px 15px rgba(76, 201, 240, 0.3);
        }

        .btn-success:hover {
            background: #3ab7de;
            box-shadow: 0 6px 20px rgba(76, 201, 240, 0.4);
        }

        .status-area {
            margin-top: 30px;
            display: none;
        }

        .status {
            padding: 20px;
            border-radius: var(--border-radius);
            margin-bottom: 20px;
            display: flex;
            align-items: center;
            gap: 15px;
        }

        .status i {
            font-size: 1.5rem;
        }

        .status-processing {
            background: #fff9e6;
            border-left: 4px solid var(--warning);
            color: #856404;
        }

        .status-success {
            background: #e8f5e9;
            border-left: 4px solid var(--success);
            color: #155724;
        }

        .status-error {
            background: #ffeaea;
            border-left: 4px solid var(--danger);
            color: #721c24;
        }

        .file-info {
            background: #f8f9fe;
            padding: 15px;
            border-radius: var(--border-radius);
            margin-top: 20px;
            display: flex;
            align-items: center;
            gap: 15px;
        }

        .file-info i {
            font-size: 1.5rem;
            color: var(--primary);
        }

        .file-details {
            flex-grow: 1;
        }

        .file-name {
            font-weight: 600;
            margin-bottom: 5px;
        }

        .file-size {
            color: var(--gray);
            font-size: 0.9rem;
        }

        .progress-bar {
            height: 6px;
            background: #e9ecef;
            border-radius: 3px;
            margin-top: 10px;
            overflow: hidden;
        }

        .progress {
            height: 100%;
            background: var(--primary);
            width: 0%;
            transition: width 0.4s ease;
        }

        .features {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-top: 40px;
        }

        .feature {
            background: white;
            border-radius: var(--border-radius);
            padding: 25px;
            text-align: center;
            box-shadow: var(--box-shadow);
            transition: var(--transition);
        }

        .feature:hover {
            transform: translateY(-5px);
        }

        .feature i {
            font-size: 2.5rem;
            color: var(--primary);
            margin-bottom: 15px;
        }

        .feature h3 {
            margin-bottom: 10px;
            color: var(--dark);
        }

        .feature p {
            color: var(--gray);
            line-height: 1.5;
        }

        .footer {
            text-align: center;
            margin-top: 50px;
            color: var(--gray);
            font-size: 0.9rem;
            padding: 20px;
        }

        @media (max-width: 768px) {
            h1 {
                font-size: 2rem;
            }
            
            .card {
                padding: 20px;
            }
            
            .features {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo">
                <i class="fas fa-brain logo-icon"></i>
            </div>
            <h1>TACTI-WAVE B-DRIVE</h1>
            <p class="subtitle">Upload images or PDF documents to extract text directly to your TACTI-WAVE device. Fast, secure, and accessible.</p>
        </div>

        <div class="card">
            <h2 class="card-title"><i class="fas fa-file-upload"></i> Upload File</h2>
            
            <div class="upload-area" id="uploadArea">
                <i class="fas fa-cloud-upload-alt"></i>
                <div class="upload-text">Drag & Drop your file here</div>
                <div class="upload-subtext">Supports JPG, PNG, PDF and other image formats</div>
                <div class="btn" id="browseBtn">
                    <i class="fas fa-folder-open"></i> Browse Files
                </div>
                <input type="file" id="fileInput" class="file-input" accept="image/*,.pdf">
            </div>

            <div class="file-info" id="fileInfo" style="display: none;">
                <i class="fas fa-file"></i>
                <div class="file-details">
                    <div class="file-name" id="fileName">document.pdf</div>
                    <div class="file-size" id="fileSize">0 KB</div>
                    <div class="progress-bar">
                        <div class="progress" id="uploadProgress"></div>
                    </div>
                </div>
                <i class="fas fa-times" id="removeFile" style="cursor: pointer;"></i>
            </div>

            <button class="btn btn-block" id="processBtn" style="display: none;">
                <i class="fas fa-cogs"></i> Extract Text
            </button>
        </div>

        <div class="status-area" id="statusArea">
            <div class="status status-processing" id="statusProcessing">
                <i class="fas fa-sync-alt fa-spin"></i>
                <div>
                    <div class="status-title">Processing your file...</div>
                    <div class="status-desc" id="statusDesc">This may take a few moments</div>
                </div>
            </div>
            
            <div class="status status-success" id="statusSuccess" style="display: none;">
                <i class="fas fa-check-circle"></i>
                <div>
                    <div class="status-title">Text extracted successfully!</div>
                    <div class="status-desc">The text has been sent to your TACTI-WAVE device</div>
                </div>
            </div>
            
            <div class="status status-error" id="statusError" style="display: none;">
                <i class="fas fa-exclamation-circle"></i>
                <div>
                    <div class="status-title">Error processing file</div>
                    <div class="status-desc" id="errorDesc">Please try again with a different file</div>
                </div>
            </div>
        </div>

        <div class="features">
            <div class="feature">
                <i class="fas fa-image"></i>
                <h3>Image OCR</h3>
                <p>Extract text from JPG, PNG, and other image formats with high accuracy.</p>
            </div>
            <div class="feature">
                <i class="fas fa-file-pdf"></i>
                <h3>PDF Processing</h3>
                <p>Direct text extraction from PDF documents without conversion needed.</p>
            </div>
            <div class="feature">
                <i class="fas fa-bolt"></i>
                <h3>Fast Processing</h3>
                <p>Quick text extraction with optimized algorithms for speedy results.</p>
            </div>
        </div>

        <div class="footer">
            <p>TACTI-WAVE B-DRIVE &copy; 2023 | Making Technology Accessible</p>
        </div>
    </div>

    <script>
        pdfjsLib.GlobalWorkerOptions.workerSrc = 'https://cdnjs.cloudflare.com/ajax/libs/pdf.js/2.16.105/pdf.worker.min.js';

        // DOM Elements
        const uploadArea = document.getElementById('uploadArea');
        const fileInput = document.getElementById('fileInput');
        const browseBtn = document.getElementById('browseBtn');
        const fileInfo = document.getElementById('fileInfo');
        const fileName = document.getElementById('fileName');
        const fileSize = document.getElementById('fileSize');
        const uploadProgress = document.getElementById('uploadProgress');
        const removeFile = document.getElementById('removeFile');
        const processBtn = document.getElementById('processBtn');
        const statusArea = document.getElementById('statusArea');
        const statusProcessing = document.getElementById('statusProcessing');
        const statusSuccess = document.getElementById('statusSuccess');
        const statusError = document.getElementById('statusError');
        const statusDesc = document.getElementById('statusDesc');
        const errorDesc = document.getElementById('errorDesc');

        let selectedFile = null;

        // Event Listeners
        browseBtn.addEventListener('click', () => fileInput.click());
        uploadArea.addEventListener('dragover', (e) => {
            e.preventDefault();
            uploadArea.style.borderColor = 'var(--primary)';
            uploadArea.style.background = '#f0f3ff';
        });
        
        uploadArea.addEventListener('dragleave', () => {
            uploadArea.style.borderColor = '#c3cfe2';
            uploadArea.style.background = '#f8f9fe';
        });
        
        uploadArea.addEventListener('drop', (e) => {
            e.preventDefault();
            uploadArea.style.borderColor = '#c3cfe2';
            uploadArea.style.background = '#f8f9fe';
            
            if (e.dataTransfer.files.length) {
                handleFileSelection(e.dataTransfer.files[0]);
            }
        });

        fileInput.addEventListener('change', (e) => {
            if (e.target.files.length) {
                handleFileSelection(e.target.files[0]);
            }
        });

        removeFile.addEventListener('click', () => {
            resetFileSelection();
        });

        processBtn.addEventListener('click', processFile);

        // Functions
        function handleFileSelection(file) {
            selectedFile = file;
            
            // Update file info
            fileName.textContent = file.name;
            fileSize.textContent = formatFileSize(file.size);
            fileInfo.style.display = 'flex';
            processBtn.style.display = 'block';
            
            // Reset progress
            uploadProgress.style.width = '0%';
        }

        function resetFileSelection() {
            selectedFile = null;
            fileInput.value = '';
            fileInfo.style.display = 'none';
            processBtn.style.display = 'none';
            hideAllStatuses();
        }

        function formatFileSize(bytes) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const sizes = ['Bytes', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        }

        function showStatus(type) {
            hideAllStatuses();
            statusArea.style.display = 'block';
            
            switch(type) {
                case 'processing':
                    statusProcessing.style.display = 'flex';
                    break;
                case 'success':
                    statusSuccess.style.display = 'flex';
                    break;
                case 'error':
                    statusError.style.display = 'flex';
                    break;
            }
        }

        function hideAllStatuses() {
            statusProcessing.style.display = 'none';
            statusSuccess.style.display = 'none';
            statusError.style.display = 'none';
            statusArea.style.display = 'none';
        }

        async function processFile() {
            if (!selectedFile) return;
            
            showStatus('processing');
            
            try {
                // Simulate upload progress
                for (let i = 0; i <= 100; i += 10) {
                    uploadProgress.style.width = i + '%';
                    await new Promise(resolve => setTimeout(resolve, 200));
                }
                
                let extractedText = '';
                
                if (selectedFile.type === 'application/pdf') {
                    statusDesc.textContent = 'Extracting text from PDF...';
                    extractedText = await extractTextFromPDF(selectedFile);
                } else if (selectedFile.type.startsWith('image/')) {
                    statusDesc.textContent = 'Sending image for OCR processing...';
                    
                    // Send image to ESP32 for OCR processing
                    const formData = new FormData();
                    formData.append('file', selectedFile);

                    const response = await fetch('/uploadImage', {
                        method: 'POST',
                        body: formData
                    });

                    if (!response.ok) {
                        throw new Error('Failed to process image');
                    }
                    
                    // For demo purposes, we'll simulate extracted text
                    extractedText = "This is simulated extracted text from the image. In the actual implementation, this would come from the OCR processing on your TACTI-WAVE device.";
                } else {
                    throw new Error('Unsupported file type');
                }
                
                if (!extractedText || extractedText.length === 0) {
                    throw new Error('No text found in file');
                }
                
                // Send extracted text to ESP32
                if (selectedFile.type.startsWith('image/')) {
                    // For images, the text is already sent via the uploadImage endpoint
                    showStatus('success');
                } else {
                    // For PDFs, send the text directly
                    const textResponse = await fetch('/sendText', {
                        method: 'POST',
                        headers: { 'Content-Type': 'text/plain' },
                        body: extractedText
                    });

                    if (textResponse.ok) {
                        showStatus('success');
                    } else {
                        throw new Error('Failed to send text to device');
                    }
                }
                
            } catch (error) {
                console.error('Error:', error);
                errorDesc.textContent = error.message;
                showStatus('error');
            }
        }

        async function extractTextFromPDF(file) {
            const arrayBuffer = await file.arrayBuffer();
            const pdf = await pdfjsLib.getDocument(arrayBuffer).promise;
            let fullText = '';

            for (let i = 1; i <= pdf.numPages; i++) {
                const page = await pdf.getPage(i);
                const textContent = await page.getTextContent();
                const pageText = textContent.items.map(item => item.str).join(' ');
                fullText += pageText + '\n';
            }

            return fullText.trim();
        }
    </script>
</body>
</html>
)rawliteral";

  bdriveServer.send(200, "text/html", html);
}

bool createDirectoryPath(const String& path) {
  if (!sdCardAvailable) return false;

  // Remove leading slash if present
  String cleanPath = path;
  if (cleanPath.startsWith("/")) {
    cleanPath = cleanPath.substring(1);
  }

  // If path is empty or root, return true
  if (cleanPath.length() == 0) return true;

  // Check if directory already exists
  if (SD.exists("/" + cleanPath)) {
    return true;
  }

  // Split path into components and create each level
  String currentPath = "";
  int startPos = 0;
  int slashPos = cleanPath.indexOf('/', startPos);

  while (slashPos != -1 || startPos < cleanPath.length()) {
    String segment;
    if (slashPos != -1) {
      segment = cleanPath.substring(startPos, slashPos);
      startPos = slashPos + 1;
      slashPos = cleanPath.indexOf('/', startPos);
    } else {
      segment = cleanPath.substring(startPos);
      startPos = cleanPath.length();
    }

    if (segment.length() > 0) {
      if (currentPath.length() > 0) {
        currentPath += "/";
      }
      currentPath += segment;

      String fullPath = "/" + currentPath;
      if (!SD.exists(fullPath.c_str())) {
        if (!SD.mkdir(fullPath.c_str())) {
          //Serial.printf("[SD] Failed to create directory: %s\n", fullPath.c_str());
          return false;
        }
        Serial.printf("[SD] Created directory: %s\n", fullPath.c_str());
      }
    }
  }

  return true;
}

void handleCreateDirectory() {
  if (sdUploadServer.hasArg("path")) {
    String dirPath = sdUploadServer.arg("path");

    if (createDirectoryPath(dirPath)) {
      Serial.printf("[SD UPLOAD] Created directory: %s\n", dirPath.c_str());
      sdUploadServer.send(200, "text/plain", "OK");
    } else {
      sdUploadServer.send(500, "text/plain", "Failed to create directory");
    }
  } else {
    sdUploadServer.send(400, "text/plain", "Missing path parameter");
  }
}


// Modified file upload handler with folder path support
void handleFileUploadToSDWithPath() {
  HTTPUpload& upload = sdUploadServer.upload();
  static File uploadFile;

  if (upload.status == UPLOAD_FILE_START) {
    // Get the folder path from the query parameter
    String folderPath = "";
    if (sdUploadServer.hasArg("folder")) {
      folderPath = sdUploadServer.arg("folder");
    }

    String filename = upload.filename;

    // Combine folder path with filename
    String fullPath = "/";
    if (folderPath.length() > 0) {
      if (!folderPath.startsWith("/")) {
        fullPath += folderPath;
      } else {
        fullPath = folderPath;
      }
      if (!fullPath.endsWith("/")) {
        fullPath += "/";
      }
    }
    fullPath += filename;

    Serial.printf("[SD UPLOAD] Receiving: %s\n", fullPath.c_str());

    if (sdCardAvailable) {
      // Extract directory path and create it if needed
      int lastSlash = fullPath.lastIndexOf('/');
      if (lastSlash > 0) {
        String dirPath = fullPath.substring(0, lastSlash);
        createDirectoryPath(dirPath);
      }

      uploadFile = SD.open(fullPath.c_str(), FILE_WRITE);

      if (!uploadFile) {
        Serial.printf("[SD UPLOAD] Failed to create file: %s\n", fullPath.c_str());
      }
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.printf("[SD UPLOAD] Saved: %s (%u bytes)\n", upload.filename.c_str(), upload.totalSize);
      sdUploadServer.send(200, "text/plain", "OK");
    } else {
      sdUploadServer.send(500, "text/plain", "Upload failed");
    }
  }
}


// Get directory tree as JSON
String getDirectoryTree(const String& path = "/", int level = 0) {
  if (!sdCardAvailable || level > 5) return "[]";  // Max 5 levels deep

  File dir = SD.open(path.c_str());
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return "[]";
  }

  String result = "[";
  bool first = true;

  File file = dir.openNextFile();
  while (file) {
    if (!first) result += ",";
    first = false;

    String name = String(file.name());
    if (name.startsWith("/")) {
      name = name.substring(1);
    }

    if (file.isDirectory()) {
      result += "{\"name\":\"" + name + "\",\"type\":\"folder\",\"size\":0}";
    } else {
      result += "{\"name\":\"" + name + "\",\"type\":\"file\",\"size\":" + String(file.size()) + "}";
    }

    file = dir.openNextFile();
  }
  dir.close();

  result += "]";
  return result;
}



void handleGetDirectoryTree() {
  String path = "/";
  if (sdUploadServer.hasArg("path")) {
    path = sdUploadServer.arg("path");
  }

  sdUploadServer.send(200, "application/json", getDirectoryTree(path));
}

String sendToOCRFromFile(const char* filepath) {
  if (!sdCardAvailable) {
    //Serial.println("[B-DRIVE] SD card not available");
    return "SD card not available";
  }

  File file = SD.open(filepath);
  if (!file) {
    //Serial.print("[B-DRIVE] Cannot open file: ");
    //Serial.println(filepath);
    return "Cannot open temp file";
  }

  size_t fileSize = file.size();
  //Serial.printf("[B-DRIVE] Sending %u bytes to OCR\n", fileSize);

  if (fileSize == 0 || fileSize > 1000000) {  // Max 1MB
    file.close();
    //Serial.println("[B-DRIVE] File size invalid");
    return "Error: File size invalid";
  }

  WiFiClientSecure client;
  client.setInsecure();

  //Serial.print("[B-DRIVE] Connecting to OCR server...");
  if (!client.connect(ocrHost, ocrPort)) {
    file.close();
    //Serial.println(" FAILED");
    return "Error: Cannot connect to OCR server";
  }
  //Serial.println(" OK");

  String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";

  String bodyStart = "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"file\"; filename=\"image.jpg\"\r\n";
  bodyStart += "Content-Type: image/jpeg\r\n\r\n";

  String bodyEnd = "\r\n--" + boundary + "\r\n";
  bodyEnd += "Content-Disposition: form-data; name=\"language\"\r\n\r\neng\r\n";
  bodyEnd += "--" + boundary + "--\r\n";

  size_t contentLength = bodyStart.length() + fileSize + bodyEnd.length();

  String request = "POST /parse/image HTTP/1.1\r\n";
  request += String("Host: ") + ocrHost + "\r\n";
  request += "User-Agent: ESP32\r\n";
  request += "Accept: application/json\r\n";
  request += String("apikey: ") + ocrApiKey + "\r\n";
  request += "Connection: close\r\n";
  request += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
  request += "Content-Length: " + String(contentLength) + "\r\n\r\n";

  //Serial.println("[B-DRIVE] Sending HTTP request...");
  client.print(request);
  client.print(bodyStart);

  // Stream file in chunks
  uint8_t buf[512];
  size_t totalSent = 0;
  while (file.available()) {
    size_t len = file.read(buf, sizeof(buf));
    client.write(buf, len);
    totalSent += len;
  }
  file.close();

  client.print(bodyEnd);
  //Serial.printf("[B-DRIVE] Sent %u bytes total\n", totalSent);

  // Wait for response
  //Serial.println("[B-DRIVE] Waiting for OCR response...");
  String response = "";
  unsigned long timeout = millis() + 20000;  // 20 second timeout

  while (client.connected() && millis() < timeout) {
    while (client.available()) {
      char c = client.read();
      response += c;
      timeout = millis() + 3000;  // Extend timeout while receiving data
    }
    delay(10);
  }

  client.stop();

  Serial.printf("[B-DRIVE] Response length: %d bytes\n", response.length());

  if (response.length() == 0) {
    //Serial.println("[B-DRIVE] Empty response from OCR server");
    return "Error: Empty response from OCR server";
  }

  // Extract JSON body
  int idx = response.indexOf("\r\n\r\n");
  String body = (idx >= 0) ? response.substring(idx + 4) : response;

  // Debug: print first 200 chars of response
  Serial.println("[B-DRIVE] Response :");
  Serial.println(body.substring(0, min(200, (int)body.length())));

  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    //Serial.print("[B-DRIVE] JSON parse error: ");
    //Serial.println(err.c_str());
    return "Error: OCR response parse failed";
  }

  // Check for API errors
  if (doc.containsKey("IsErroredOnProcessing")) {
    bool isError = doc["IsErroredOnProcessing"];
    if (isError) {
      const char* errorMsg = doc["ErrorMessage"][0] | "Unknown error";
      //Serial.print("[B-DRIVE] OCR API error: ");
      //Serial.println(errorMsg);
      return String("Error: ") + String(errorMsg);
    }
  }

  // Extract parsed text
  if (doc.containsKey("ParsedResults") && doc["ParsedResults"].size() > 0) {
    const char* parsedText = doc["ParsedResults"][0]["ParsedText"];
    if (parsedText && strlen(parsedText) > 0) {
      //Serial.println("[B-DRIVE] OCR successful");
      return String(parsedText);
    }
  }

  //Serial.println("[B-DRIVE] No text detected in image");
  return "No text detected in image";
}
void handleImageUpload() {
  HTTPUpload& upload = bdriveServer.upload();
  static File tempFile;

  if (upload.status == UPLOAD_FILE_START) {
    //Serial.printf("[B-DRIVE] Image upload start: %s\n", upload.filename.c_str());

    if (sdCardAvailable) {
      tempFile = SD.open("/temp_upload.jpg", FILE_WRITE);
      if (!tempFile) {
        //Serial.println("[B-DRIVE] Failed to create temp file");
      }
    }

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (tempFile) {
      size_t written = tempFile.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        //Serial.println("[B-DRIVE] Write error during upload");
      }
    }

  } else if (upload.status == UPLOAD_FILE_END) {
    if (tempFile) {
      tempFile.flush();
      tempFile.close();
      //Serial.printf("[B-DRIVE] Image upload finished (%u bytes)\n", upload.totalSize);

      // Process image with OCR
      String ocrResult = sendToOCRFromFile("/temp_upload.jpg");

      if (ocrResult.startsWith("Error:") || ocrResult.startsWith("SD card")) {
        //Serial.print("[B-DRIVE] OCR failed: ");
        //Serial.println(ocrResult);
        bdriveServer.send(500, "text/plain", "OCR processing failed");
      } else {
        //Serial.println("[B-DRIVE] OCR successful, sending text to handleTextReceive");
        playWAV("/TACTI_VISION_WAV/OCR_Completed.wav");
        // Store extracted text and trigger save workflow
        bdriveExtractedText = ocrResult;
        bdriveTextReady = true;
        bdriveWaitingForSave = true;
        bdriveNaming = false;
        currentWord = "";

        Serial.println("\n========== EXTRACTED TEXT (OCR) ==========");
        Serial.println(bdriveExtractedText);
        Serial.println("====================================\n");
        //Serial.println("[B-DRIVE] Text received. Waiting for save decision...");
        Serial.println("[B-DRIVE] Press BACKSPACE + CTRL to save or PREVIOUS to discard");
        vibe(0.3);

        bdriveServer.send(200, "text/plain", "OK");
      }

      // Clean up temp file
      if (sdCardAvailable && SD.exists("/temp_upload.jpg")) {
        SD.remove("/temp_upload.jpg");
      }
    } else {
      //Serial.println("[B-DRIVE] No temp file to process");
      bdriveServer.send(500, "text/plain", "Upload failed");
    }
  }
}

void handleTextReceive() {
  if (bdriveServer.hasArg("plain")) {
    bdriveExtractedText = bdriveServer.arg("plain");
  } else {
    // Read from request body
    bdriveExtractedText = bdriveServer.arg("plain");
  }

  Serial.println("\n========== EXTRACTED TEXT ==========");
  Serial.println(bdriveExtractedText);
  Serial.println("====================================\n");

  bdriveTextReady = true;
  bdriveWaitingForSave = true;
  bdriveNaming = false;
  currentWord = "";

  //Serial.println("[B-DRIVE] Text received. Waiting for save decision...");
  Serial.println("[B-DRIVE] Press BACKSPACE + CTRL to save or PREVIOUS to discard");
  vibe(0.3);

  bdriveServer.send(200, "text/plain", "OK");
}


String sendToOCR(uint8_t* imageData, size_t imageLen) {
  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect(ocrHost, ocrPort)) {
    //Serial.println("[B-DRIVE] OCR server connection failed");
    return "Error: Cannot connect to OCR server";
  }

  String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";

  String bodyStart = "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"file\"; filename=\"upload.jpg\"\r\n";
  bodyStart += "Content-Type: image/jpeg\r\n\r\n";

  String bodyEnd = "\r\n--" + boundary + "\r\n";
  bodyEnd += "Content-Disposition: form-data; name=\"language\"\r\n\r\n";
  bodyEnd += "eng\r\n";
  bodyEnd += "--" + boundary + "--\r\n";

  size_t contentLength = bodyStart.length() + imageLen + bodyEnd.length();

  String request = "POST /parse/image HTTP/1.1\r\n";
  request += String("Host: ") + ocrHost + "\r\n";
  request += "User-Agent: TACTI-WAVE\r\n";
  request += "Accept: application/json\r\n";
  request += String("apikey: ") + ocrApiKey + "\r\n";
  request += "Connection: close\r\n";
  request += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
  request += "Content-Length: " + String(contentLength) + "\r\n\r\n";

  client.print(request);
  client.print(bodyStart);
  client.write(imageData, imageLen);
  client.print(bodyEnd);

  String response = "";
  unsigned long timeout = millis() + 15000;

  while (client.connected() && millis() < timeout) {
    while (client.available()) {
      char c = client.read();
      response += c;
      timeout = millis() + 2000;
    }
  }

  client.stop();

  int idx = response.indexOf("\r\n\r\n");
  String body = (idx >= 0) ? response.substring(idx + 4) : response;

  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    //Serial.println("[B-DRIVE] JSON parse error");
    return "Error: OCR response parse failed";
  }

  const char* parsedText = doc["ParsedResults"][0]["ParsedText"];
  return parsedText ? String(parsedText) : "No text detected in image";
}

void startBdriveServer() {
  if (!ensureWiFiConnected()) {
    //Serial.println("[B-DRIVE] WiFi not connected");
    return;
  }

  bdriveServer.on("/", HTTP_GET, handleBdriveRoot);
  bdriveServer.on(
    "/uploadImage", HTTP_POST,
    []() {
      bdriveServer.send(200);
    },
    handleImageUpload);
  bdriveServer.on("/sendText", HTTP_POST, handleTextReceive);

  bdriveServer.begin();
  //Serial.println("[B-DRIVE] Web server started");
  Serial.print("[B-DRIVE] Access Webserver at: http://");
  Serial.println(WiFi.localIP());
}

void saveGeminiFile(const String& filename, const String& content) {
  if (!sdCardAvailable) {
    //Serial.println("[GEMINI] SD card not available");
    vibe(0.1);
    return;
  }

  String fn = normalizeSdPath(filename);
  File file = SD.open(fn.c_str(), FILE_WRITE);
  if (!file) {
    //Serial.print("[GEMINI] Failed to open file: ");
    //Serial.println(fn);
    vibe(0.1);
    return;
  }

  size_t written = file.print(content);
  file.flush();
  file.close();

  if (written > 0) {
    Serial.print("[GEMINI] Successfully saved ");
    Serial.print(written);
    Serial.print(" bytes to ");
    Serial.println(fn);
    playWAV("/TACTI_VISION_WAV/SAVED.wav");
    vibe(0.3);
  } else {
    //Serial.println("[GEMINI] Write failed");
    vibe(0.1);
  }
}

void stopBdriveServer() {
  bdriveServer.stop();
  //Serial.println("[B-DRIVE] Web server stopped");
}


String generateUniqueBdriveName() {
  if (!sdCardAvailable) return String("Bdrive1.txt");

  for (int i = 1; i < 1000; i++) {
    String filename = String("Bdrive") + String(i) + String(".txt");
    if (!SD.exists(filename.c_str()) && !SD.exists((String("/") + filename).c_str())) {
      return filename;
    }
  }
  return String("Bdrive_unknown.txt");
}

void saveBdriveFile(const String& filename, const String& content) {
  if (!sdCardAvailable) {
    //Serial.println("[B-DRIVE] SD card not available");
    vibe(0.1);
    return;
  }

  String fn = normalizeSdPath(filename);
  File file = SD.open(fn.c_str(), FILE_WRITE);
  if (!file) {
    //Serial.print("[B-DRIVE] Failed to open file: ");
    //Serial.println(fn);
    vibe(0.1);
    return;
  }

  size_t written = file.print(content);
  file.flush();
  file.close();

  if (written > 0) {
    Serial.print("[B-DRIVE] Successfully saved ");
    Serial.print(written);
    Serial.print(" bytes to ");
    Serial.println(fn);
    playWAV("/TACTI_VISION_WAV/SAVED.wav");
    vibe(0.3);
  } else {
    //Serial.println("[B-DRIVE] Write failed");
    vibe(0.1);
  }
}

//These functions help to avoid the conflicts due to the usage of I2C and I2S simultaneously -> Since they share same resource on ESP32C6
// ==== Properly cleanup audio after playback ====
void cleanupAudio() {
  //Serial.println("🔧 Cleaning up I2S...");

  if (audioOut) {
    audioOut->stop();  // Stop I2S output
    delay(100);        // Allow I2S to fully stop
  }
  //Serial.println("✅ I2S stopped.");
}

// ==== Function: Reinitialize I2C after I2S usage ====
void reinitI2C() {
  //Serial.println("🔧 Reinitializing I2C...");

  // *** Properly end I2C before restarting ***
  Wire.end();
  delay(150);  // Increased delay for stability with multiple I2C devices

  // Restart I2C bus
  Wire.begin(PCF8575_SDA, PCF8575_SCL);
  Wire.setClock(100000);  // 100kHz for stability with multiple devices
  delay(150);             // Allow bus to stabilize

  // *** Reinitialize PCF8575 ***
  writePCF8575(0xFFFF);  // Reset PCF8575 to input mode
  delay(50);

  // *** Test PCF8575 connection ***
  uint16_t testPCF = readPCF8575();
  if (testPCF == 0xFFFF) {
    //Serial.println("[PCF8575] ✅ Reconnected after I2S");
  } else {
    //Serial.println("[PCF8575] ⚠️  Unexpected state after I2S");
  }

  // *** Reinitialize DigiSpark if it was active ***
  if (hidInitialized) {
    //Serial.println("[HID] Checking DigiSpark after I2C reinit...");
    delay(100);  // Extra delay for DigiSpark

    Wire.beginTransmission(DIGISPARK_ADDR);
    uint8_t error = Wire.endTransmission();

    if (error != 0) {
      //Serial.println("[HID] ⚠️  DigiSpark needs reconnection");

      // Try to reconnect
      delay(200);
      Wire.beginTransmission(DIGISPARK_ADDR);
      error = Wire.endTransmission();

      if (error == 0) {
        //Serial.println("[HID] ✅ DigiSpark reconnected successfully");
      } else {
        //Serial.println("[HID] ❌ DigiSpark reconnection failed - HID disabled");
        hidInitialized = false;
        hidKeyboardEnabled = false;
      }
    } else {
      //Serial.println("[HID] ✅ DigiSpark still connected");
    }
  }

  //Serial.println("✅ I2C reinitialized.\n");
}

// ==== URL Encode Helper for VoiceRSS ====
String urlEncode(String str) {
  String encoded = "";
  char c;
  char code0;
  char code1;

  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encoded += '+';
    } else if (isalnum(c)) {
      encoded += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}

// ==== Read File Content as String ====
String readFileContent(const char* filepath) {
  if (!sdCardAvailable) {
    //Serial.println("[TTS] SD card not available");
    return "";
  }

  String normalized = normalizeSdPath(String(filepath));
  File file = SD.open(normalized.c_str());

  if (!file) {
    //Serial.print("[TTS] Cannot open file: ");
    //Serial.println(normalized);
    return "";
  }

  String content = "";
  while (file.available()) {
    content += (char)file.read();
  }
  file.close();

  Serial.printf("[TTS] Read %d characters from %s\n", content.length(), normalized.c_str());
  return content;
}

// ==== Fetch TTS from VoiceRSS and save to SD ====
bool fetchTTSFromVoiceRSS(const char* text, const char* filename, bool isWAV = true) {
  if (!ensureWiFiConnected()) {
    Serial.println("[TTS] WiFi not connected!");
    vibe(0.1);
    return false;
  }

  HTTPClient http;

  // Build VoiceRSS API URL with parameters
  String url = String(VOICERSS_URL) + "?key=" + VOICERSS_API_KEY;
  url += "&src=" + urlEncode(text);
  url += "&hl=en-us";  // Language: English (US)

  if (isWAV) {
    url += "&c=WAV";               // Format: WAV
    url += "&f=22khz_16bit_mono";  // ⚡ RELIABLE: Good quality, smaller files, better headers
  } else {
    url += "&c=MP3";               // Format: MP3
    url += "&f=22khz_16bit_mono";  // ⚡ RELIABLE: Good quality, smaller files
  }

  //Serial.print("[TTS] API URL: ");
  //Serial.println(url);

  http.begin(url);
  http.setTimeout(15000);  // 15 second timeout
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int len = http.getSize();
    //Serial.printf("[TTS] Response size: %d bytes\n", len);

    // Delete existing file
    if (SD.exists(filename)) {
      SD.remove(filename);
    }

    // Create file on SD card
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
      //Serial.println("[TTS] Failed to create file on SD!");
      http.end();
      return false;
    }

    // Get stream
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buff[512] = { 0 };
    int bytesWritten = 0;

    // Read and write in chunks
    while (http.connected() && (len > 0 || len == -1)) {
      size_t size = stream->available();
      if (size) {
        int c = stream->readBytes(buff, min((size_t)sizeof(buff), size));
        file.write(buff, c);
        bytesWritten += c;

        if (len > 0) {
          len -= c;
        }

        Serial.print(".");
      }
      delay(1);
    }

    Serial.println();
    //Serial.printf("[TTS] ✅ Written %d bytes to %s\n", bytesWritten, filename);

    file.close();
    http.end();
    return true;

  } else {
    //Serial.printf("[TTS] ❌ HTTP Error: %d\n", httpCode);
    if (httpCode > 0) {
      String payload = http.getString();
      //Serial.println("[TTS] Response: " + payload);
    }
    http.end();
    return false;
  }
}

// ==================== HID SHORTCUT FUNCTIONS ====================

// Send 16-bit command to DigiSpark for shortcuts
bool sendHIDShortcutCommand(uint8_t cmd) {
  if (!hidInitialized) {
    //Serial.println("[HID] DigiSpark not initialized");
    return false;
  }

  delay(5);
  Wire.beginTransmission(DIGISPARK_ADDR);
  Wire.write(0x12);  // Magic byte for shortcut commands
  Wire.write(0x34);  // Second magic byte
  Wire.write(cmd);   // Actual command
  uint8_t error = Wire.endTransmission();

  if (error != 0) {
    //Serial.printf("[HID] Shortcut send failed (error %d)\n", error);
    return false;
  }

  delay(10);
  return true;
}

// Execute selected HID shortcut
void executeHIDShortcut(int shortcutIndex) {
  //Serial.print("[HID SHORTCUT] Executing: ");
  Serial.println(shortcutNames[shortcutIndex]);

  vibe(0.2);

  bool success = false;

  switch (shortcutIndex) {
    case 0:  // WiFi
      Serial.println("[HID] Opening WiFi settings...");
      success = sendHIDShortcutCommand(HID_CMD_OPEN_WIFI);
      break;

    case 1:  // CHROME
      Serial.println("[HID] Opening Chrome...");
      success = sendHIDShortcutCommand(HID_CMD_OPEN_CHROME);
      break;

    case 2:  // Gmail
      Serial.println("[HID] Opening Gmail...");
      success = sendHIDShortcutCommand(HID_CMD_OPEN_GMAIL);
      break;

    case 3:  // ChatGPT
      Serial.println("[HID] Opening ChatGPT...");
      success = sendHIDShortcutCommand(HID_CMD_OPEN_CHATGPT);
      break;

    case 4:  // Python Compiler
      Serial.println("[HID] Opening Python Compiler...");
      success = sendHIDShortcutCommand(HID_CMD_OPEN_PYCOMPILER);
      break;

    case 5:  // Library
      Serial.println("[HID] Opening Library...");
      success = sendHIDShortcutCommand(HID_CMD_OPEN_LIBRARY);
      break;
  }

  if (success) {
    //Serial.println("[HID] Command sent successfully!");
    playWAV("/TACTI_VISION_WAV/SELECTED.wav");
    vibe(0.3);
  } else {
    //Serial.println("[HID] Command failed!");
    vibe(0.1);
  }
}

// ==================== OTA UPDATE FUNCTIONS ====================

String getOTAFirmwareInfo(String url) {
  HTTPClient http;
  http.setTimeout(30000);
  http.begin(url);
  int httpCode = http.GET();
  String payload = "";

  if (httpCode == HTTP_CODE_OK) {
    payload = http.getString();
  } else {
    //Serial.printf("[OTA] HTTP Error: %d\n", httpCode);
  }

  http.end();
  return payload;
}

bool isNewOTAVersionAvailable(String jsonData, String& newVersion, String& fwUrl) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    //Serial.println("[OTA] JSON parse failed!");
    return false;
  }

  newVersion = doc["latest_version"].as<String>();
  fwUrl = doc["url"].as<String>();

  if (newVersion != String(CURRENT_FIRMWARE_VERSION)) {
    return true;
  }
  return false;
}

bool downloadAndUpdateOTA(const char* url) {
  HTTPClient http;
  http.begin(url);
  playWAV("/TACTI_VISION_WAV/Downloading.wav");
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    //Serial.printf("[OTA] Download Error: %d\n", httpCode);
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    //Serial.println("[OTA] Invalid firmware size");
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  //Serial.printf("[OTA] Firmware size: %d bytes\n", contentLength);

  if (!Update.begin(contentLength)) {
    Update.printError(Serial);
    http.end();
    return false;
  }

  //Serial.println("[OTA] Writing firmware...");
  size_t written = Update.writeStream(*stream);

  if (written == contentLength && Update.end(true)) {
    Serial.printf("[OTA] ✅ Successfully written %u bytes\n", written);
    http.end();
    return true;
  } else {
    Serial.println("[OTA] ❌ Update failed!");
    Update.printError(Serial);
    http.end();
    return false;
  }
}

void checkForOTAUpdate() {
  Serial.println("[OTA] Checking for firmware updates...");
  playWAV("/TACTI_VISION_WAV/Firmware_check.wav");

  if (!ensureWiFiConnected()) {
    //Serial.println("[OTA] WiFi connection failed");
    vibe(0.15);
    playWAV("/TACTI_VISION_WAV/PREVIOUS.wav");
    enterMode(MODE_OPTIONS);
    return;
  }

  vibe(0.2);
  //Serial.println("[OTA] Fetching update info...");

  String json = getOTAFirmwareInfo(latestInfoURL);
  if (json == "") {
    //Serial.println("[OTA] Failed to fetch update info");
    vibe(0.15);
    delay(1000);
    enterMode(MODE_OPTIONS);
    return;
  }

  if (isNewOTAVersionAvailable(json, otaNewVersion, otaFirmwareURL)) {
    Serial.println("========================================");
    Serial.printf("[OTA] ✨ New firmware available: %s\n", otaNewVersion.c_str());
    Serial.printf("[OTA] Current version: %s\n", CURRENT_FIRMWARE_VERSION);
    Serial.println("========================================");
    playWAV("/TACTI_VISION_WAV/firmware_available.wav");
    Serial.println("[OTA] Press SELECT to update");
    Serial.println("[OTA] Press PREVIOUS to skip");

    otaUpdateAvailable = true;
    otaWaitingForConfirm = true;
    otaConfirmTimeout = millis() + OTA_CONFIRM_TIMEOUT;
    vibe(0.3);
    playWAV("/TACTI_VISION_WAV/SELECTED.wav");
  } else {
    Serial.println("[OTA] ✅ You're running the latest version");
    Serial.printf("[OTA] Current: %s\n", CURRENT_FIRMWARE_VERSION);
    playWAV("/TACTI_VISION_WAV/latestversion.wav");
    vibe(0.2);
    delay(2000);
    enterMode(MODE_OPTIONS);
  }
}

void performOTAUpdate() {
  Serial.println("[OTA] Starting firmware update...");
  Serial.println("[OTA] ⚠️  DO NOT POWER OFF THE DEVICE!");
  vibe(0.5);

  if (downloadAndUpdateOTA(otaFirmwareURL.c_str())) {
    Serial.println("[OTA] ✅ Update successful!");
    Serial.println("[OTA] Rebooting in 2 seconds...");
    playWAV("/TACTI_VISION_WAV/update_sucess.wav");
    vibe(0.8);
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("[OTA] Update failed!");
    vibe(0.15);
    delay(2000);

    // Reset state
    otaUpdateAvailable = false;
    otaWaitingForConfirm = false;
    enterMode(MODE_OPTIONS);
  }
}

void cancelOTAUpdate() {
  Serial.println("[OTA] Update cancelled");
  otaUpdateAvailable = false;
  otaWaitingForConfirm = false;
  vibe(0.12);
  playWAV("/TACTI_VISION_WAV/PREVIOUS.wav");
  enterMode(MODE_OPTIONS);
}

// ==================== SD UPLOAD MODE FUNCTIONS ====================

String getSDFileList() {
  String fileList = "[";
  if (!sdCardAvailable) {
    return "[]";
  }

  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    return "[]";
  }

  bool first = true;
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      if (!first) fileList += ",";
      first = false;

      String name = String(file.name());
      if (name.charAt(0) == '/') {
        name = name.substring(1);
      }

      fileList += "{\"name\":\"" + name + "\",\"size\":" + String(file.size()) + "}";
    }
    file = root.openNextFile();
  }
  root.close();

  fileList += "]";
  return fileList;
}

void handleSDUploadRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TACTI-WAVE SD Upload Portal</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <style>
        :root {
            --primary: #4361ee;
            --primary-dark: #3a56d4;
            --success: #4cc9f0;
            --danger: #e63946;
            --warning: #f8961e;
            --light: #f8f9fa;
            --dark: #212529;
            --gray: #6c757d;
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        }

        body {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            color: var(--dark);
        }

        .container {
            max-width: 1000px;
            margin: 0 auto;
        }

        .header {
            text-align: center;
            color: white;
            margin-bottom: 30px;
        }

        .header h1 {
            font-size: 2.5rem;
            margin-bottom: 10px;
        }

        .header p {
            font-size: 1.1rem;
            opacity: 0.9;
        }

        .card {
            background: white;
            border-radius: 15px;
            padding: 30px;
            margin-bottom: 20px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
        }

        .card-title {
            font-size: 1.5rem;
            margin-bottom: 20px;
            color: var(--primary);
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .upload-options {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-bottom: 20px;
        }

        .upload-area {
            border: 3px dashed #c3cfe2;
            border-radius: 12px;
            padding: 30px;
            text-align: center;
            cursor: pointer;
            transition: all 0.3s;
            background: #f8f9fe;
        }

        .upload-area:hover {
            border-color: var(--primary);
            background: #f0f3ff;
            transform: translateY(-2px);
        }

        .upload-area i {
            font-size: 3rem;
            color: var(--primary);
            margin-bottom: 10px;
        }

        .upload-area h3 {
            font-size: 1.1rem;
            margin-bottom: 8px;
        }

        .upload-area p {
            font-size: 0.9rem;
            color: var(--gray);
        }

        .btn {
            background: var(--primary);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 50px;
            font-size: 1rem;
            cursor: pointer;
            transition: all 0.3s;
            display: inline-flex;
            align-items: center;
            gap: 8px;
        }

        .btn:hover {
            background: var(--primary-dark);
            transform: translateY(-2px);
        }

        .btn-danger {
            background: var(--danger);
        }

        .btn-danger:hover {
            background: #d62839;
        }

        .file-list {
            margin-top: 20px;
        }

        .file-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 15px;
            background: #f8f9fe;
            border-radius: 8px;
            margin-bottom: 10px;
        }

        .file-info {
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .file-info i.fa-folder {
            color: #f39c12;
        }

        .file-info i.fa-file {
            color: var(--primary);
        }

        .file-actions {
            display: flex;
            gap: 10px;
        }

        .status {
            padding: 15px;
            border-radius: 8px;
            margin-top: 15px;
            display: none;
        }

        .status.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }

        .status.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }

        .status.info {
            background: #d1ecf1;
            color: #0c5460;
            border: 1px solid #bee5eb;
        }

        .progress-bar {
            height: 6px;
            background: #e9ecef;
            border-radius: 3px;
            margin-top: 10px;
            overflow: hidden;
            display: none;
        }

        .progress {
            height: 100%;
            background: var(--primary);
            width: 0%;
            transition: width 0.3s;
        }

        .file-input {
            display: none;
        }

        @media (max-width: 768px) {
            .upload-options {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1><i class="fas fa-sd-card"></i> TACTI-WAVE SD Upload</h1>
            <p>Upload files and folders to your device's SD card</p>
        </div>

        <div class="card">
            <h2 class="card-title"><i class="fas fa-cloud-upload-alt"></i> Upload Options</h2>
            
            <div class="upload-options">
                <div class="upload-area" id="fileUploadArea">
                    <i class="fas fa-file-upload"></i>
                    <h3>Upload Files</h3>
                    <p>Select multiple files</p>
                    <input type="file" id="fileInput" class="file-input" multiple>
                </div>
                
                <div class="upload-area" id="folderUploadArea">
                    <i class="fas fa-folder-open"></i>
                    <h3>Upload Folder</h3>
                    <p>Select entire folder</p>
                    <input type="file" id="folderInput" class="file-input" webkitdirectory directory multiple>
                </div>
            </div>

            <div class="progress-bar" id="progressBar">
                <div class="progress" id="progress"></div>
            </div>

            <div class="status" id="status"></div>
        </div>

        <div class="card">
            <h2 class="card-title"><i class="fas fa-folder-open"></i> SD Card Contents</h2>
            <button class="btn" onclick="refreshFileList()">
                <i class="fas fa-sync-alt"></i> Refresh
            </button>
            <div class="file-list" id="fileList">
                <p style="text-align: center; color: var(--gray);">Loading...</p>
            </div>
        </div>
    </div>

    <script>
        const fileUploadArea = document.getElementById('fileUploadArea');
        const folderUploadArea = document.getElementById('folderUploadArea');
        const fileInput = document.getElementById('fileInput');
        const folderInput = document.getElementById('folderInput');
        const progressBar = document.getElementById('progressBar');
        const progress = document.getElementById('progress');
        const status = document.getElementById('status');
        const fileList = document.getElementById('fileList');

        fileUploadArea.onclick = () => fileInput.click();
        folderUploadArea.onclick = () => folderInput.click();

        fileUploadArea.ondragover = (e) => {
            e.preventDefault();
            fileUploadArea.style.borderColor = 'var(--primary)';
        };

        fileUploadArea.ondragleave = () => {
            fileUploadArea.style.borderColor = '#c3cfe2';
        };

        fileUploadArea.ondrop = (e) => {
            e.preventDefault();
            fileUploadArea.style.borderColor = '#c3cfe2';
            
            if (e.dataTransfer.files.length) {
                uploadFiles(e.dataTransfer.files);
            }
        };

        fileInput.onchange = (e) => {
            if (e.target.files.length) {
                uploadFiles(e.target.files);
            }
        };

        folderInput.onchange = (e) => {
            if (e.target.files.length) {
                uploadFolder(e.target.files);
            }
        };

        async function uploadFiles(files) {
            progressBar.style.display = 'block';
            progress.style.width = '0%';
            showStatus('Uploading files...', 'info');

            let uploaded = 0;
            let failed = 0;

            for (let i = 0; i < files.length; i++) {
                const file = files[i];
                const formData = new FormData();
                formData.append('file', file);

                try {
                    const response = await fetch('/uploadToSD', {
                        method: 'POST',
                        body: formData
                    });

                    progress.style.width = ((i + 1) / files.length * 100) + '%';

                    if (response.ok) {
                        uploaded++;
                    } else {
                        failed++;
                        console.error('Failed to upload:', file.name);
                    }
                } catch (error) {
                    failed++;
                    console.error('Error uploading:', file.name, error);
                }
            }

            setTimeout(() => {
                progressBar.style.display = 'none';
                progress.style.width = '0%';
                
                if (failed === 0) {
                    showStatus('Successfully uploaded ' + uploaded + ' file(s)', 'success');
                } else {
                    showStatus('Uploaded ' + uploaded + ' file(s), ' + failed + ' failed', 'error');
                }
                
                refreshFileList();
            }, 500);
        }

        async function uploadFolder(files) {
            progressBar.style.display = 'block';
            progress.style.width = '0%';
            showStatus('Uploading folder structure...', 'info');

            const folderPaths = new Set();
            for (let file of files) {
                const path = file.webkitRelativePath || file.relativePath || '';
                if (path) {
                    const parts = path.split('/');
                    let currentPath = '';
                    for (let i = 0; i < parts.length - 1; i++) {
                        currentPath += (i > 0 ? '/' : '') + parts[i];
                        folderPaths.add(currentPath);
                    }
                }
            }

            for (let folderPath of folderPaths) {
                try {
                    const response = await fetch('/createDirectory?path=' + encodeURIComponent(folderPath), {
                        method: 'POST'
                    });
                    if (!response.ok) {
                        console.error('Failed to create directory:', folderPath);
                    }
                } catch (error) {
                    console.error('Error creating directory:', folderPath, error);
                }
            }

            let uploaded = 0;
            let failed = 0;

            for (let i = 0; i < files.length; i++) {
                const file = files[i];
                const relativePath = file.webkitRelativePath || file.relativePath || file.name;
                
                const lastSlash = relativePath.lastIndexOf('/');
                const folderPath = lastSlash > 0 ? relativePath.substring(0, lastSlash) : '';
                const fileName = lastSlash > 0 ? relativePath.substring(lastSlash + 1) : relativePath;

                const formData = new FormData();
                formData.append('file', new File([file], fileName, { type: file.type }));

                try {
                    const url = '/uploadToSD' + (folderPath ? '?folder=' + encodeURIComponent(folderPath) : '');
                    const response = await fetch(url, {
                        method: 'POST',
                        body: formData
                    });

                    progress.style.width = ((i + 1) / files.length * 100) + '%';

                    if (response.ok) {
                        uploaded++;
                    } else {
                        failed++;
                        console.error('Failed to upload:', relativePath);
                    }
                } catch (error) {
                    failed++;
                    console.error('Error uploading:', relativePath, error);
                }
            }

            setTimeout(() => {
                progressBar.style.display = 'none';
                progress.style.width = '0%';
                
                if (failed === 0) {
                    showStatus('Successfully uploaded folder with ' + uploaded + ' file(s)', 'success');
                } else {
                    showStatus('Uploaded ' + uploaded + ' file(s), ' + failed + ' failed', 'error');
                }
                
                refreshFileList();
            }, 500);
        }

        function showStatus(message, type) {
            status.textContent = message;
            status.className = 'status ' + type;
            status.style.display = 'block';
            
            if (type === 'success' || type === 'error') {
                setTimeout(() => {
                    status.style.display = 'none';
                }, 5000);
            }
        }

        async function refreshFileList() {
            try {
                const response = await fetch('/getDirectoryTree');
                const items = await response.json();

                if (items.length === 0) {
                    fileList.innerHTML = '<p style="text-align: center; color: var(--gray);">No files on SD card</p>';
                    return;
                }

                fileList.innerHTML = items.map(item => `
                    <div class="file-item">
                        <div class="file-info">
                            <i class="fas fa-${item.type === 'folder' ? 'folder' : 'file'}"></i>
                            <div>
                                <strong>${item.name}</strong>
                                ${item.type === 'file' ? '<br><small>' + formatFileSize(item.size) + '</small>' : ''}
                            </div>
                        </div>
                        <div class="file-actions">
                            ${item.type === 'file' ? `
                                <button class="btn" onclick="downloadFile('${item.name}')">
                                    <i class="fas fa-download"></i>
                                </button>
                            ` : ''}
                            <button class="btn btn-danger" onclick="deleteFile('${item.name}')">
                                <i class="fas fa-trash"></i>
                            </button>
                        </div>
                    </div>
                `).join('');
            } catch (error) {
                fileList.innerHTML = '<p style="text-align: center; color: var(--danger);">Error loading files</p>';
            }
        }

        function formatFileSize(bytes) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const sizes = ['Bytes', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        }

        function downloadFile(filename) {
            window.location.href = '/download?file=' + encodeURIComponent(filename);
        }

        async function deleteFile(filename) {
            if (!confirm('Delete ' + filename + '?')) return;

            try {
                const response = await fetch('/delete?file=' + encodeURIComponent(filename), {
                    method: 'DELETE'
                });

                if (response.ok) {
                    showStatus('Deleted: ' + filename, 'success');
                    refreshFileList();
                } else {
                    showStatus('Failed to delete: ' + filename, 'error');
                }
            } catch (error) {
                showStatus('Error: ' + error.message, 'error');
            }
        }

        refreshFileList();
    </script>
</body>
</html>
)rawliteral";

  sdUploadServer.send(200, "text/html", html);
}


void handleFileUploadToSD() {
  HTTPUpload& upload = sdUploadServer.upload();
  static File uploadFile;

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("[SD UPLOAD] Receiving: %s\n", upload.filename.c_str());

    if (sdCardAvailable) {
      String filename = "/" + String(upload.filename.c_str());
      uploadFile = SD.open(filename.c_str(), FILE_WRITE);

      if (!uploadFile) {
        //Serial.println("[SD UPLOAD] Failed to create file");
      }
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.printf("[SD UPLOAD] Saved: %s (%u bytes)\n", upload.filename.c_str(), upload.totalSize);
      playWAV("/TACTI_VISION_WAV/File_Upload.wav");
      sdUploadServer.send(200, "text/plain", "OK");
    } else {
      sdUploadServer.send(500, "text/plain", "Upload failed");
    }
  }
}

void handleDeleteFile() {
  if (sdUploadServer.hasArg("file")) {
    String filename = sdUploadServer.arg("file");
    String path = normalizeSdPath(filename);

    if (SD.exists(path.c_str())) {
      SD.remove(path.c_str());
      Serial.printf("[SD UPLOAD] Deleted: %s\n", filename.c_str());
      sdUploadServer.send(200, "text/plain", "OK");
    } else {
      sdUploadServer.send(404, "text/plain", "File not found");
    }
  } else {
    sdUploadServer.send(400, "text/plain", "Missing filename");
  }
}

void handleDownloadFile() {
  if (sdUploadServer.hasArg("file")) {
    String filename = sdUploadServer.arg("file");
    String path = normalizeSdPath(filename);

    if (SD.exists(path.c_str())) {
      File file = SD.open(path.c_str());
      if (file) {
        sdUploadServer.streamFile(file, "application/octet-stream");
        file.close();
        Serial.printf("[SD UPLOAD] Downloaded: %s\n", filename.c_str());
        return;
      }
    }
    sdUploadServer.send(404, "text/plain", "File not found");
  } else {
    sdUploadServer.send(400, "text/plain", "Missing filename");
  }
}

void startSDUploadServer() {
  if (!ensureWiFiConnected()) {
    //Serial.println("[SD UPLOAD] WiFi not connected");
    return;
  }

  sdUploadServer.on("/", HTTP_GET, handleSDUploadRoot);

  // Use the handler with folder path support
  sdUploadServer.on(
    "/uploadToSD", HTTP_POST,
    []() {
      sdUploadServer.send(200);
    },
    handleFileUploadToSDWithPath);  // ← Changed from handleFileUploadToSD

  // Add endpoint for directory creation
  sdUploadServer.on("/createDirectory", HTTP_POST, handleCreateDirectory);

  sdUploadServer.on("/getFileList", HTTP_GET, []() {
    sdUploadServer.send(200, "application/json", getSDFileList());
  });

  // Add directory tree endpoint
  sdUploadServer.on("/getDirectoryTree", HTTP_GET, handleGetDirectoryTree);

  sdUploadServer.on("/delete", HTTP_DELETE, handleDeleteFile);
  sdUploadServer.on("/download", HTTP_GET, handleDownloadFile);

  sdUploadServer.begin();
  sdUploadServerActive = true;

  Serial.println("[SD UPLOAD] Server started on port 8080");
  Serial.print("[SD UPLOAD] Access at: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":8080");
}

void stopSDUploadServer() {
  if (sdUploadServerActive) {
    sdUploadServer.stop();
    sdUploadServerActive = false;
    //Serial.println("[SD UPLOAD] Server stopped");
  }
}

// Function to play alphabet audio files
void playAlphabetAudio(char letter) {
  if (!sdCardAvailable) {
    return;
  }

  // Convert letter to uppercase
  if (letter >= 'a' && letter <= 'z') {
    letter = toupper(letter);
  }

  // Only play for valid letters A-Z
  if (letter < 'A' || letter > 'Z') {
    return;
  }

  // Build file path: /Alphabets/A.wav, /Alphabets/B.wav, etc.
  String audioPath = String("/Alphabets/") + String(letter) + String(".wav");

  //Serial.print("[ALPHABET AUDIO] Playing: ");
  //Serial.println(audioPath);

  // Play the audio file
  playWAV(audioPath.c_str());
}

// Toggle alphabet audio feedback on/off
void toggleAlphabetAudio() {
  alphabetAudioEnabled = !alphabetAudioEnabled;

  if (alphabetAudioEnabled) {
    Serial.println("[ALPHABET AUDIO] ENABLED");
    playWAV("/TACTI_VISION_WAV/Alpha_enab.wav");
    vibe(0.3);
  } else {
    Serial.println("[ALPHABET AUDIO] DISABLED");
    playWAV("/TACTI_VISION_WAV/Alpha_disable.wav");
    vibe(0.2);
  }
}

// ==================== AUDIO PLAYER MODE FUNCTIONS ====================

void listAudioFiles(const char* dirname, std::vector<String>& files) {
  files.clear();
  if (!sdCardAvailable) {
    Serial.println("[AUDIO PLAYER] SD card not available");
    return;
  }

  Serial.printf("[AUDIO PLAYER] Listing audio files in: %s\n", dirname);
  File dir = SD.open(dirname);

  if (!dir || !dir.isDirectory()) {
    //Serial.println("[AUDIO PLAYER] Failed to open AudioFiles directory");
    if (dir) dir.close();
    return;
  }

  File file = dir.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String name = String(file.name());

      // Only include .wav files
      if (name.endsWith(".wav") || name.endsWith(".WAV")) {
        // Remove leading slash if present
        if (name.charAt(0) == '/') {
          name = name.substring(1);
        }

        files.push_back(name);
        Serial.printf(" AUDIO: %s (%u bytes)\n", name.c_str(), file.size());
      }
    }
    file = dir.openNextFile();
  }
  dir.close();

  // Sort files alphabetically
  std::sort(files.begin(), files.end());

  //Serial.printf("[AUDIO PLAYER] Found %d audio file(s)\n", files.size());
}

void playCurrentAudioFile() {
  if (audioFiles.empty() || !sdCardAvailable) {
    //Serial.println("[AUDIO PLAYER] No files available");
    vibe(0.15);
    return;
  }

  String selectedFile = audioFiles[currentAudioIndex];
  String audioPath = "/AudioFiles/" + selectedFile;

  Serial.print("[AUDIO PLAYER] Playing: ");
  Serial.println(selectedFile);

  audioPlaying = true;
  vibe(0.3);

  playWAV(audioPath.c_str());

  audioPlaying = false;
  //Serial.println("[AUDIO PLAYER] Playback finished");
}

void stopCurrentAudio() {
  if (audioPlaying && wav && wav->isRunning()) {
    wav->stop();
    audioPlaying = false;
    Serial.println("[AUDIO PLAYER] Playback stopped");
    playWAV("/TACTI_VISION_WAV/playbackstopped.wav");
    vibe(0.2);
  }
}

void nextAudioFile() {
  if (audioFiles.empty()) return;

  if (currentAudioIndex < audioFiles.size() - 1) {
    currentAudioIndex++;
  } else {
    currentAudioIndex = 0;  // Loop to first
  }

  Serial.print("[AUDIO PLAYER] >> ");
  Serial.println(audioFiles[currentAudioIndex]);
  vibe(0.08);
}

void previousAudioFile() {
  if (audioFiles.empty()) return;

  if (currentAudioIndex > 0) {
    currentAudioIndex--;
  } else {
    currentAudioIndex = audioFiles.size() - 1;  // Loop to last
  }

  Serial.print("[AUDIO PLAYER] << ");
  Serial.println(audioFiles[currentAudioIndex]);
  vibe(0.08);
}

// ==================== TEXT AUTO-CORRECTION FUNCTIONS ====================

String correctWord(const String& word) {
  String lowerWord = word;
  lowerWord.toLowerCase();

  if (correctionMap.count(lowerWord)) {
    Serial.print("[CORRECTION] ");
    Serial.print(word);
    Serial.print(" -> ");
    Serial.println(correctionMap[lowerWord]);
    return correctionMap[lowerWord];
  }

  return word;  // No correction needed
}

String correctText(const String& text) {
  if (text.length() == 0) return text;

  String correctedText = "";
  String currentWord = "";
  int correctionsCount = 0;

  for (int i = 0; i < text.length(); i++) {
    char c = text.charAt(i);

    // Check if character is a word separator
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '.' || c == ',' || c == '!' || c == '?' || c == ';' || c == ':' || c == '"' || c == '\'') {

      if (currentWord.length() > 0) {
        String corrected = correctWord(currentWord);
        if (corrected != currentWord) {
          correctionsCount++;
        }
        correctedText += corrected;
        currentWord = "";
      }
      correctedText += c;
    } else {
      currentWord += c;
    }
  }

  // Handle last word
  if (currentWord.length() > 0) {
    String corrected = correctWord(currentWord);
    if (corrected != currentWord) {
      correctionsCount++;
    }
    correctedText += corrected;
  }

  if (correctionsCount > 0) {
    Serial.printf("[CORRECTION] Made %d correction(s)\n", correctionsCount);
    vibe(0.2);
  }

  return correctedText;
}

void applyTextCorrection() {
  if (currentWord.length() == 0) {
    Serial.println("[CORRECTION] No text to correct");
    vibe(0.1);
    return;
  }
  playWAV("/TACTI_VISION_WAV/Autocorrection.wav");
  Serial.println("\n========== ORIGINAL TEXT ==========");
  Serial.println(currentWord.c_str());
  Serial.println("===================================\n");

  String corrected = correctText(String(currentWord.c_str()));

  Serial.println("\n========== CORRECTED TEXT ==========");
  Serial.println(corrected);
  Serial.println("====================================\n");

  // Update current word with corrected version
  currentWord = std::string(corrected.c_str());

  vibe(0.3);
}

// ==== DEEP SLEEP FUNCTIONS ====
void enterDeepSleep() {
  Serial.println("\n[DEEP SLEEP] Preparing to enter deep sleep...");

  // Play deep sleep announcement FIRST
  playWAV("/TACTI_VISION_WAV/Deepsleep.wav");

  // Add delay to ensure audio playback completes
  delay(1000);

  // **FIX: Vibrate BEFORE cleanup, not during/after**
  vibe(0.5);
  delay(600);  // Wait for vibration to complete (0.5s + buffer)

  // Clean up resources
  if (currentAppMode == B_DRIVE_MODE) {
    stopBdriveServer();
  }
  if (currentAppMode == SD_UPLOAD_MODE) {
    stopSDUploadServer();
  }
  if (notifyActive) {
    stopNotificationBLE();
  }

  // Stop audio
  cleanupAudio();

  // **FIX: Ensure vibration motor is OFF before sleep**
  digitalWrite(VIB_PIN, LOW);
  pinMode(VIB_PIN, INPUT_PULLDOWN);  // Optional: set to input to prevent floating

  Serial.println("[DEEP SLEEP] Entering deep sleep mode...");
  Serial.println("[DEEP SLEEP] Power cycle device to wake up");
  Serial.flush();

  delay(200);  // Final delay to ensure all operations complete

  // Enter deep sleep without any wakeup source
  esp_deep_sleep_start();
}

void checkDeepSleep() {
  // Skip if in exempt mode
  if (isDeepSleepExemptMode()) {
    deepSleepEnabled = false;
    return;
  }

  // Enable deep sleep monitoring for other modes
  deepSleepEnabled = true;

  // Check if timeout exceeded
  if (millis() - lastActivityTime > DEEP_SLEEP_TIMEOUT) {
    Serial.println("[DEEP SLEEP] Inactivity timeout reached");
    enterDeepSleep();
  }
}
// ===== URL ENCODE HELPER FOR TWILIO =====
String urlEncodeForSMS(const String& str) {
  String encoded = "";
  char buf[5];
  for (uint16_t i = 0; i < str.length(); i++) {
    unsigned char c = str[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += (char)c;
    } else {
      sprintf(buf, "%%%02X", c);
      encoded += buf;
    }
  }
  return encoded;
}
// ===== SEND SOS SMS VIA TWILIO =====
bool sendSOSSMS() {
  Serial.println("\n[SOS] 🚨 EMERGENCY ALERT TRIGGERED!");

  // Play SOS audio FIRST (before WiFi connection)
  if (sdCardAvailable && SD.exists("/TACTI_VISION_WAV/SOS.wav")) {
    playWAV("/TACTI_VISION_WAV/SOS.wav");
  }
  vibe(0.5);

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SOS] WiFi not connected - connecting now...");

    if (!ensureWiFiConnected()) {
      //Serial.println("[SOS] ❌ WiFi connection FAILED - cannot send SMS");
      vibe(0.2);  // Error vibration
      delay(200);
      vibe(0.2);
      return false;
    }

    Serial.println("[SOS] ✅ WiFi connected successfully");
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  // Twilio API endpoint
  String url = String("https://api.twilio.com/2010-04-01/Accounts/")
               + twilio_account_sid + "/Messages.json";

  //Serial.println("[SOS] Connecting to Twilio API...");

  if (!http.begin(client, url)) {
    //Serial.println("[SOS] ❌ Failed to initiate HTTPS connection");
    vibe(0.15);
    return false;
  }

  // Set authorization
  http.setAuthorization(twilio_account_sid, twilio_auth_token);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setTimeout(15000);  // 15 second timeout

  // Build SMS message with timestamp
  unsigned long uptimeSeconds = millis() / 1000;
  unsigned long hours = uptimeSeconds / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  unsigned long seconds = uptimeSeconds % 60;

  String smsBody =
    "\n\n🚨 SOS TRIGGERED FROM STELLAR VISION\n"
    "STATUS: Emergency\n"
    "DEVICE: TACTI-WAVE\n"
    "UPTIME: "
    + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s\n"
                                                                        "⚠️ IMMEDIATE ASSISTANCE REQUIRED";

  // URL encode the POST body
  String postBody =
    "To=" + urlEncodeForSMS(twilio_to_number) + "&From=" + urlEncodeForSMS(twilio_from_number) + "&Body=" + urlEncodeForSMS(smsBody);

  Serial.println("[SOS] Sending emergency SMS...");
  Serial.printf("[SOS] To: %s\n", twilio_to_number);

  int httpCode = http.POST(postBody);

  bool success = false;

  if (httpCode == 201) {
    Serial.println("[SOS] ✅ Emergency SMS sent successfully!");
    success = true;
    vibe(0.3);  // Success vibration
  } else if (httpCode > 0) {
    //Serial.printf("[SOS] ❌ HTTP Error: %d\n", httpCode);
    //Serial.println("[SOS] Response: " + http.getString());

    if (httpCode == 401) {
      //Serial.println("[SOS] ERROR: Invalid Twilio credentials (check SID/Token)");
    } else if (httpCode == 400) {
      //Serial.println("[SOS] ERROR: Invalid request (check phone numbers format)");
    } else if (httpCode >= 500) {
      //Serial.println("[SOS] ERROR: Twilio server error - try again");
    }

    vibe(0.15);  // Failure vibration
  } else {
    //Serial.printf("[SOS] ❌ Connection Error: %d\n", httpCode);
    //Serial.println("[SOS] Possible causes: No internet, DNS failure, or timeout");
    vibe(0.15);
  }

  http.end();

  if (success) {
    Serial.println("[SOS] SMS DELIVERED - Help is on the way!");
  }

  return success;
}



void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // ==== INIT I2C FIRST ====
  Wire.begin(PCF8575_SDA, PCF8575_SCL);
  Wire.setClock(100000);  // Set I2C to 100kHz for stability
  delay(100);             // Give I2C bus time to stabilize
  //Serial.println("[I2C] Bus initialized on pins SDA=0, SCL=1");
  // *** Configure I2C for multiple devices with audio ***
  Wire.setTimeOut(500);  // 500ms timeout for I2C operationsi
  //Serial.println("[I2C] Timeout configured for stability");

  Serial.println("[HID]✅ Initializing keyboard support...");
  initHIDKeyboard();
  hidInitialized = false;
  hidKeyboardEnabled = false;
  // ==== INIT PCF8575 ====
  writePCF8575(0xFFFF);  // Set all pins as inputs -> (pull-up mode)
  delay(50);

  uint16_t testRead = readPCF8575();
  if (testRead == 0xFFFF) {
    Serial.println("[PCF8575] ✅ Initialized successfully!");
  } else {
    Serial.println("[PCF8575] ⚠️  Warning: unexpected initial state");
  }

  // Initialize state arrays
  for (int i = 0; i < NUM_BRAILLE_PINS; i++) {
    lastBrailleStates[i] = HIGH;
    currentBrailleStates[i] = HIGH;
  }

  // Initialize key states
  lastCtrlState = HIGH;
  currentCtrlState = HIGH;
  lastBackspaceState = HIGH;
  currentBackspaceState = HIGH;
  lastSpaceLeftState = HIGH;
  currentSpaceLeftState = HIGH;
  lastSpaceRightState = HIGH;
  currentSpaceRightState = HIGH;
  lastPrevState = HIGH;
  currentPrevState = HIGH;
  lastUpState = HIGH;
  currentUpState = HIGH;
  lastSelectState = HIGH;
  currentSelectState = HIGH;
  lastDownState = HIGH;
  currentDownState = HIGH;

  pinMode(VIB_PIN, OUTPUT);
  populateBrailleMaps();
  populateCorrectionMap();

  // ==== INIT SD CARD ====
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SPI, 40000000)) {
    Serial.println("[SD] ❌ Card initialization FAILED!");
    sdCardAvailable = false;
  } else {
    Serial.println("[SD] ✅ Card initialized successfully!");
    sdCardAvailable = true;

    if (SD.exists("/NOTIFY.txt")) {
      SD.remove("/NOTIFY.txt");
      Serial.println("[SETUP] Cleared old /NOTIFY.txt");
    }

    if (SD.exists("/message.txt")) {
      Serial.println("Removing old message.txt for cleanup");
      SD.remove("/message.txt");
    }
  }
  // Check if Alphabets folder exists
  if (sdCardAvailable && SD.exists("/Alphabets")) {
    //Serial.println("[ALPHABET AUDIO] Folder found!");
    alphabetAudioEnabled = false;  // Start disabled by default
    //Serial.println("[ALPHABET AUDIO] Disabled by default (long press UP to toggle)");
  } else {
    //Serial.println("[ALPHABET AUDIO] ⚠️ /Alphabets folder not found!");
    alphabetAudioEnabled = false;
  }
  // ==== INIT MPU6050 (uses existing I2C bus) ====
  if (!mpu.begin(MPU_ADDR, &Wire)) {
    Serial.println("[MPU6050] ❌ Initialization failed! (Gesture mode disabled)");
  } else {
    Serial.println("[MPU6050] ✅ Initialized successfully");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }

  // Ensure I2C clock is set correctly after MPU init
  Wire.setClock(100000);
  delay(50);

  // ==== INIT I2S AUDIO for MAX98357A ====
  audioOut = new AudioOutputI2S();
  audioOut->SetPinout(I2S_BCLK, I2S_LRC, I2S_DIN);
  audioOut->SetGain(0.9);
  Serial.println("[AUDIO] ✅ System initialized!");

  // ==== INIT NOTIFICATION RING BUFFER ====
  noInterrupts();
  notif_head = 0;
  notif_tail = 0;
  memset(notif_ring, 0, sizeof(notif_ring));
  interrupts();
  //Serial.println("[NOTIFY] Ring buffer cleared");

  // ==== INIT CHRONOS BLE ====
  watch.setConnectionCallback(chronosConnectionCallback);
  watch.setNotificationCallback(chronosNotificationCallback);
  watch.setRingerCallback(chronosRingerCallback);

  watch.begin();
  bleInitialized = true;
  //Serial.print("[CHRONOS] Local address: ");
  //Serial.println(watch.getAddress());

  vibe(0.3);
  playWAV("/TACTI_VISION_WAV/StellarVision.wav");
  Serial.println("\nStellar Vision V1  \n");

  currentAppMode = NORMAL_MODE;
  enterMode(NORMAL_MODE);
  lastActivityTime = millis();
  deepSleepEnabled = false;
}

void processMPU6050Gestures() {
  if (!gestureModeActive) {
    return;
  }
  updateActivityTimer();
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float ax = a.acceleration.x - ACCEL_X_OFFSET;
  float ay = a.acceleration.y - ACCEL_Y_OFFSET;
  float az = a.acceleration.z - ACCEL_Z_OFFSET;

  unsigned long currentTime = millis();

  // RIGHT tilt = SELECT (Pin 12)
  if (ay > TILT_THRESHOLD) {
    if (currentTime - lastTiltTime > TILT_DEBOUNCE) {
      //Serial.println("[GESTURE] RIGHT -> SELECT");
      //Serial.println("RIGHT");
      vibe(0.1);

      // Simulate SELECT button press
      if (currentAppMode == MODE_OPTIONS) {
        if (selectedModeIndex == 0) {
          if (!notifyActive) {
            startNotificationBLE();
            Serial.println("NOTIFY turned ON");
          } else {
            stopNotificationBLE();
            Serial.println("NOTIFY turned OFF");
          }
          vibe(0.2);
        } else {
          //Serial.print("SELECTED: ");
          //Serial.println(modeNames[selectedModeIndex]);
          playWAV("/TACTI_VISION_WAV/SELECTED.wav");
          vibe(0.3);
          enterMode(modeOptionValues[selectedModeIndex]);
        }
      } else if (currentAppMode == SD_NAVIGATION_MODE) {
        if (!sdFiles.empty() && sdCardAvailable) {
          String filePath = sdFiles[currentFileIndex];
          //Serial.print("Opening: ");
          //Serial.println(filePath);
          String normalized = normalizeSdPath(filePath);
          File fileToRead = SD.open(normalized.c_str());
          if (fileToRead) {
            Serial.println("--- FILE CONTENT ---");
            while (fileToRead.available()) {
              Serial.write(fileToRead.read());
            }
            Serial.println("\n--- END OF FILE ---");
            fileToRead.close();
          }
        }
      }

      lastTiltTime = currentTime;
    }
  }

  // LEFT tilt = PREVIOUS (Pin 10)
  else if (ay < -TILT_THRESHOLD) {
    if (currentTime - lastTiltTime > TILT_DEBOUNCE) {
      //Serial.println("[GESTURE] LEFT -> PREVIOUS");
      //Serial.println("LEFT");
      vibe(0.1);

      if (currentAppMode == MODE_OPTIONS) {
        // From MODE OPTIONS, go to PERKINS
        //Serial.println("[GESTURE] Going to PERKINS MODE");
        playWAV("/TACTI_VISION_WAV/PERKINS_MODE.wav");
        vibe(0.2);
        enterMode(PERKINS_MODE);
      } else if (currentAppMode != NORMAL_MODE && currentAppMode != BOOT_MODE) {
        // From other modes, go to MODE OPTIONS
        //Serial.println("[GESTURE] Going to MODE OPTIONS");
        playWAV("/TACTI_VISION_WAV/PREVIOUS.wav");
        vibe(0.2);

        if (currentAppMode == B_DRIVE_MODE) {
          stopBdriveServer();
        }

        enterMode(MODE_OPTIONS);
      }

      lastTiltTime = currentTime;
    }
  }
  // DOWN tilt = DOWN navigation (Pin 13)
  else if (ax > TILT_THRESHOLD) {
    if (currentTime - lastTiltTime > TILT_DEBOUNCE) {
      //Serial.println("DOWN");
      vibe(0.08);

      if (currentAppMode == MODE_OPTIONS) {
        if (selectedModeIndex < NUM_MODE_OPTIONS - 1) selectedModeIndex++;
        else selectedModeIndex = 0;
        Serial.print(">> ");
        Serial.println(modeNames[selectedModeIndex]);
        switch (selectedModeIndex) {
          case 0: playWAV("/TACTI_VISION_WAV/NOTIFY.wav"); break;
          case 1: playWAV("/TACTI_VISION_WAV/GEMINI_AI.wav"); break;
          case 2: playWAV("/TACTI_VISION_WAV/NOTE_MAKER.wav"); break;
          case 3: playWAV("/TACTI_VISION_WAV/HID_SHORTCUT.wav"); break;
          case 4: playWAV("/TACTI_VISION_WAV/B_DRIVE.wav"); break;
          case 5: playWAV("/TACTI_VISION_WAV/SD_MODE.wav"); break;
          case 6: playWAV("/TACTI_VISION_WAV/SD_UPLOAD.wav"); break;
          case 7: playWAV("/TACTI_VISION_WAV/SYSTEM_UPDATE.wav"); break;
          case 8: playWAV("/TACTI_VISION_WAV/AUDIO_PLAYER.wav"); break;
        }
      } else if (currentAppMode == SD_NAVIGATION_MODE) {
        if (!sdFiles.empty() && currentFileIndex < sdFiles.size() - 1) {
          currentFileIndex++;
          Serial.print("Current: ");
          Serial.println(sdFiles[currentFileIndex]);
        }
      }

      lastTiltTime = currentTime;
    }
  }
  // UP tilt = UP navigation (Pin 11) - REMOVED AUTO-DISABLE FEATURE
  else if (ax < -TILT_THRESHOLD) {
    if (currentTime - lastTiltTime > TILT_DEBOUNCE) {
      //Serial.println("UP");
      vibe(0.08);

      if (currentAppMode == MODE_OPTIONS) {
        if (selectedModeIndex > 0) selectedModeIndex--;
        else selectedModeIndex = NUM_MODE_OPTIONS - 1;
        Serial.print(">> ");
        Serial.println(modeNames[selectedModeIndex]);
        switch (selectedModeIndex) {
          case 0: playWAV("/TACTI_VISION_WAV/NOTIFY.wav"); break;
          case 1: playWAV("/TACTI_VISION_WAV/GEMINI_AI.wav"); break;
          case 2: playWAV("/TACTI_VISION_WAV/NOTE_MAKER.wav"); break;
          case 3: playWAV("/TACTI_VISION_WAV/HID_SHORTCUT.wav"); break;
          case 4: playWAV("/TACTI_VISION_WAV/B_DRIVE.wav"); break;
          case 5: playWAV("/TACTI_VISION_WAV/SD_MODE.wav"); break;
          case 6: playWAV("/TACTI_VISION_WAV/SD_UPLOAD.wav"); break;
          case 7: playWAV("/TACTI_VISION_WAV/SYSTEM_UPDATE.wav"); break;
          case 8: playWAV("/TACTI_VISION_WAV/AUDIO_PLAYER.wav"); break;
        }
      } else if (currentAppMode == SD_NAVIGATION_MODE) {
        if (!sdFiles.empty() && currentFileIndex > 0) {
          currentFileIndex--;
          Serial.print("Current: ");
          Serial.println(sdFiles[currentFileIndex]);
        }
      }

      lastTiltTime = currentTime;
    }
  }
}

// Handle SELECT button (Pin 12) - replaces ENTER functionality
void handleSelectButton() {
  if (millis() < suppressSingleUntil) {
    lastSelectState = currentSelectState;
    return;
  }

  bool currentReading = currentSelectState;
  unsigned long currentTime = millis();

  if (currentReading == LOW && lastSelectState == HIGH) {
    selectPressStartTime = currentTime;
  }

  if (currentReading == HIGH && lastSelectState == LOW) {
    unsigned long pressDuration = currentTime - selectPressStartTime;

    if (pressDuration < DEBOUNCE_DELAY) {
      // Ignored
    } else if (pressDuration >= LONG_PRESS_SELECT_DURATION) {
      // Long press: Enter MODE OPTIONS
      //Serial.println("SELECT Long Press! Opening Mode Options.");
      playWAV("/TACTI_VISION_WAV/MODES.wav");
      vibe(0.2);
      enterMode(MODE_OPTIONS);
    } else {
      // ===== SHORT PRESS LOGIC =====

      // === NORMAL MODE: Enter PERKINS ===
      if (currentAppMode == NORMAL_MODE) {
        //Serial.println("SELECT Tapped! Entering PERKINS MODE.");
        enterMode(PERKINS_MODE);
      }

      // === MODE OPTIONS: Select mode or toggle NOTIFY ===
      else if (currentAppMode == MODE_OPTIONS) {
        if (selectedModeIndex == 0) {
          if (!notifyActive) {
            startNotificationBLE();
            Serial.println("NOTIFY turned ON");
          } else {
            stopNotificationBLE();
            Serial.println("NOTIFY turned OFF");
          }
          vibe(0.2);
        } else {
          Serial.println(modeNames[selectedModeIndex]);
          playWAV("/TACTI_VISION_WAV/SELECTED.wav");
          vibe(0.3);
          enterMode(modeOptionValues[selectedModeIndex]);
        }
      }

      // === SYSTEM UPDATE MODE: Confirm update ===  // NEW
      else if (currentAppMode == SYSTEM_UPDATE_MODE) {
        if (otaWaitingForConfirm && otaUpdateAvailable) {
          performOTAUpdate();
        }
      } else if (currentAppMode == AUDIO_PLAYER_MODE) {
        if (!audioFiles.empty()) {
          if (audioPlaying) {
            stopCurrentAudio();
          } else {
            playCurrentAudioFile();
          }
        }
      }
      // === SD NAVIGATION: Open file ===
      else if (currentAppMode == SD_NAVIGATION_MODE) {
        if (!sdFiles.empty() && sdCardAvailable) {
          String filePath = sdFiles[currentFileIndex];
          //Serial.print("Opening: ");
          //Serial.println(filePath);
          String normalized = normalizeSdPath(filePath);
          File fileToRead = SD.open(normalized.c_str());
          if (fileToRead) {
            Serial.println("--- FILE CONTENT ---");
            while (fileToRead.available()) {
              Serial.write(fileToRead.read());
            }
            Serial.println("\n--- END OF FILE ---");
            fileToRead.close();
          }
        }
      }

      // === GEMINI AI: Save response ===
      else if (currentAppMode == GEMINI_AI_MODE_PLACEHOLDER) {
        if (geminiWaitingForSave) {
          // Start naming process
          //Serial.println("[GEMINI] Starting naming process...");
          Serial.println("[GEMINI-AI] Type file name (max 12 chars) or press SELECT again for default");
          geminiNaming = true;
          geminiWaitingForSave = false;
          currentWord = "";
          vibe(0.15);
          lastSelectTapTime = currentTime;
        } else if (geminiNaming) {
          // Confirm name (double tap within timeout)
          if (currentTime - lastSelectTapTime < 500) {  // 500ms double-tap window
            String name = String(currentWord.c_str());
            if (name.length() == 0) {
              geminiFileName = generateUniqueGeminiName();
              Serial.print("[GEMINI] Using default name: ");
              Serial.println(geminiFileName);
            } else if (name.length() > 12) {
              Serial.println("[GEMINI] Name too long (max 12 chars)");
              vibe(0.08);
              lastSelectTapTime = currentTime;
              return;
            } else {
              geminiFileName = name + ".txt";
              Serial.print("[GEMINI] File name set to: ");
              Serial.println(geminiFileName);
            }

            // Save the file
            saveGeminiFile(geminiFileName, geminiResponseText);
            geminiNaming = false;
            geminiResponseText = "";
            geminiFileName = "";
            currentWord = "";
            vibe(0.3);
          } else {
            // Single press in naming mode: treat as confirmation request
            lastSelectTapTime = currentTime;
          }
        }
      }

      // === NOTE MAKER: Naming workflow ===
      else if (currentAppMode == NOTE_MAKER_MODE) {
        if (isNaming) {
          // Confirm name (double tap within timeout)
          if (currentTime - lastSelectTapTime < 500) {  // 500ms double-tap window
            String name = String(currentWord.c_str());
            if (name.length() == 0) {
              newNoteFileName = generateUniqueNoteName();
              Serial.print("Using default name: ");
              Serial.println(newNoteFileName);
            } else if (name.length() > 12) {
              Serial.println("Name too long (max 12 chars)");
              vibe(0.08);
              lastSelectTapTime = currentTime;
              return;
            } else {
              newNoteFileName = name + ".txt";
              Serial.print("Note name set to: ");
              Serial.println(newNoteFileName);
            }
            isNaming = false;
            currentWord = "";
            Serial.println("You can now start typing your note content.");
            playWAV("/TACTI_VISION_WAV/hold_save.wav");
            Serial.println("Press Ctrl+Backspace to save when done.");
            vibe(0.2);
          } else {
            // Single press: prompt for double-tap
            lastSelectTapTime = currentTime;
            Serial.println("Press SELECT again to confirm name");
          }
        }
      }

      // === B-DRIVE: Naming workflow ===
      else if (currentAppMode == B_DRIVE_MODE) {
        if (bdriveWaitingForSave) {
          //Serial.println("[B-DRIVE] Starting naming process...");
          Serial.println("[B-DRIVE] Type file name (max 12 chars) or press SELECT again for default");
          bdriveNaming = true;
          bdriveWaitingForSave = false;
          currentWord = "";
          vibe(0.15);
          lastSelectTapTime = currentTime;
        } else if (bdriveNaming) {
          if (currentTime - lastSelectTapTime < 500) {
            String name = String(currentWord.c_str());
            if (name.length() == 0) {
              bdriveFileName = generateUniqueBdriveName();
              Serial.print("[B-DRIVE] Using default name: ");
              Serial.println(bdriveFileName);
            } else if (name.length() > 12) {
              Serial.println("[B-DRIVE] Name too long (max 12 chars)");
              vibe(0.08);
              lastSelectTapTime = currentTime;
              return;
            } else {
              bdriveFileName = name + ".txt";
              Serial.print("[B-DRIVE] File name set to: ");
              Serial.println(bdriveFileName);
            }
            bdriveNaming = false;
            currentWord = "";
            Serial.println("[B-DRIVE] Press Ctrl+Backspace to save");
            vibe(0.2);
          } else {
            lastSelectTapTime = currentTime;
            Serial.println("Press SELECT again to confirm name");
          }
        }
      }
      // === HID SHORTCUT: Execute selected shortcut ===
      else if (currentAppMode == HID_SHORTCUT_MODE_PLACEHOLDER) {
        //Serial.print("EXECUTING SHORTCUT: ");
        //Serial.println(shortcutNames[selectedShortcutIndex]);
        executeHIDShortcut(selectedShortcutIndex);
      }
    }
  }

  lastSelectState = currentReading;
}

// Handle PREVIOUS button (Pin 10)
void handlePrevButton() {
  if (millis() < suppressSingleUntil) {
    lastPrevState = currentPrevState;
    return;
  }

  bool currentReading = currentPrevState;
  unsigned long currentTime = millis();

  if (currentReading == LOW && lastPrevState == HIGH) {
    prevPressStartTime = currentTime;
  }

  if (currentReading == HIGH && lastPrevState == LOW) {
    unsigned long pressDuration = currentTime - prevPressStartTime;

    if (pressDuration < DEBOUNCE_DELAY) {
      // Ignored
    } else if (pressDuration >= LONG_PRESS_PREV_DURATION) {
      // Long press: Toggle ONE HAND MODE
      if (currentAppMode == PERKINS_MODE || currentAppMode == NORMAL_MODE || currentAppMode == MODE_OPTIONS) {
        gestureModeActive = !gestureModeActive;
        if (gestureModeActive) {
          Serial.println("ONE HAND MODE ENABLED");
          playWAV("/TACTI_VISION_WAV/ONE_HAND_MODE.wav");
          vibe(0.25);
        } else {
          Serial.println("ONE HAND MODE DISABLED");
          vibe(0.15);
        }
      }
    } else {
      // Short press: Navigate backwards
      if (currentAppMode == MODE_OPTIONS) {
        //Serial.println("PREV: Going to PERKINS MODE");
        playWAV("/TACTI_VISION_WAV/PERKINS_MODE.wav");
        vibe(0.2);
        enterMode(PERKINS_MODE);
      }
      // === SYSTEM UPDATE MODE: Cancel update ===  // NEW
      else if (currentAppMode == SYSTEM_UPDATE_MODE) {
        if (otaWaitingForConfirm) {
          cancelOTAUpdate();
        } else {
          enterMode(MODE_OPTIONS);
        }
      } else if (currentAppMode == HID_SHORTCUT_MODE_PLACEHOLDER) {
        //Serial.println("PREV: Going to MODE OPTIONS");
        playWAV("/TACTI_VISION_WAV/PREVIOUS.wav");
        vibe(0.2);
        enterMode(MODE_OPTIONS);
      } else if (currentAppMode == AUDIO_PLAYER_MODE) {
        //Serial.println("[AUDIO PLAYER] Exiting to MODE OPTIONS");
        stopCurrentAudio();
        audioPlayerActive = false;
        playWAV("/TACTI_VISION_WAV/PREVIOUS.wav");
        vibe(0.2);
        enterMode(MODE_OPTIONS);
      } else if (currentAppMode == SD_UPLOAD_MODE) {
        //Serial.println("PREV: Going to MODE OPTIONS");
        stopSDUploadServer();
        playWAV("/TACTI_VISION_WAV/PREVIOUS.wav");
        vibe(0.2);
        enterMode(MODE_OPTIONS);
      } else if (currentAppMode != NORMAL_MODE && currentAppMode != BOOT_MODE && currentAppMode != PERKINS_MODE) {
        // From other modes, go to MODE OPTIONS
        //Serial.println("PREV: Going to MODE OPTIONS");
        playWAV("/TACTI_VISION_WAV/PREVIOUS.wav");
        vibe(0.2);

        if (currentAppMode == B_DRIVE_MODE) {
          stopBdriveServer();
        }

        enterMode(MODE_OPTIONS);
      }
    }
  }

  lastPrevState = currentReading;
}

// Handle UP button (Pin 11)
void handleUpButton() {
  static unsigned long lastUpPress = 0;
  static unsigned long upPressStart = 0;
  static bool upHeld = false;

  // Detect button press start
  if (currentUpState == LOW && lastUpState == HIGH) {
    upPressStart = millis();
    upHeld = true;
  }

  // Detect button release
  if (currentUpState == HIGH && lastUpState == LOW) {
    unsigned long pressDuration = millis() - upPressStart;
    upHeld = false;

    // Long press (>1 second) = Toggle alphabet audio
    if (pressDuration >= 1000) {
      toggleAlphabetAudio();
      lastUpPress = millis();
      lastUpState = currentUpState;
      return;
    }

    // Short press = Normal UP navigation
    if (millis() - lastUpPress > DEBOUNCE_DELAY) {
      //Serial.println("UP");
      vibe(0.08);

      if (currentAppMode == MODE_OPTIONS) {
        if (selectedModeIndex > 0) selectedModeIndex--;
        else selectedModeIndex = NUM_MODE_OPTIONS - 1;
        Serial.print(">> ");
        Serial.println(modeNames[selectedModeIndex]);
        switch (selectedModeIndex) {
          case 0: playWAV("/TACTI_VISION_WAV/NOTIFY.wav"); break;
          case 1: playWAV("/TACTI_VISION_WAV/GEMINI_AI.wav"); break;
          case 2: playWAV("/TACTI_VISION_WAV/NOTE_MAKER.wav"); break;
          case 3: playWAV("/TACTI_VISION_WAV/HID_SHORTCUT.wav"); break;
          case 4: playWAV("/TACTI_VISION_WAV/B_DRIVE.wav"); break;
          case 5: playWAV("/TACTI_VISION_WAV/SD_MODE.wav"); break;
          case 6: playWAV("/TACTI_VISION_WAV/SD_UPLOAD.wav"); break;
          case 7: playWAV("/TACTI_VISION_WAV/SYSTEM_UPDATE.wav"); break;
          case 8: playWAV("/TACTI_VISION_WAV/AUDIO_PLAYER.wav"); break;
        }
      } else if (currentAppMode == SD_NAVIGATION_MODE) {
        if (!sdFiles.empty() && currentFileIndex > 0) {
          currentFileIndex--;
          Serial.print("Current: ");
          Serial.println(sdFiles[currentFileIndex]);
        }
      } else if (currentAppMode == HID_SHORTCUT_MODE_PLACEHOLDER) {
        if (selectedShortcutIndex > 0) selectedShortcutIndex--;
        else selectedShortcutIndex = NUM_SHORTCUTS - 1;
        Serial.print(">> ");
        Serial.println(shortcutNames[selectedShortcutIndex]);
        switch (selectedShortcutIndex) {
          case 0: playWAV("/TACTI_VISION_WAV/WIFI.wav"); break;
          case 1: playWAV("/TACTI_VISION_WAV/CHROME.wav"); break;
          case 2: playWAV("/TACTI_VISION_WAV/GMAIL.wav"); break;
          case 3: playWAV("/TACTI_VISION_WAV/CHATGPT.wav"); break;
          case 4: playWAV("/TACTI_VISION_WAV/PY_COMPILER.wav"); break;
          case 5: playWAV("/TACTI_VISION_WAV/LIBRARY.wav"); break;
        }
      }
      if (currentAppMode == AUDIO_PLAYER_MODE) {
        if (audioPlaying) {
          stopCurrentAudio();
        } else {
          previousAudioFile();
        }
      }
      lastUpPress = millis();
    }
  }

  lastUpState = currentUpState;
}

// Handle DOWN button (Pin 13)
void handleDownButton() {
  static unsigned long lastDownPress = 0;

  if (currentDownState == LOW && lastDownState == HIGH) {
    if (millis() - lastDownPress > DEBOUNCE_DELAY) {
      //Serial.println("DOWN");
      vibe(0.08);

      if (currentAppMode == MODE_OPTIONS) {
        if (selectedModeIndex < NUM_MODE_OPTIONS - 1) selectedModeIndex++;
        else selectedModeIndex = 0;
        Serial.print(">> ");
        Serial.println(modeNames[selectedModeIndex]);
        switch (selectedModeIndex) {
          case 0: playWAV("/TACTI_VISION_WAV/NOTIFY.wav"); break;
          case 1: playWAV("/TACTI_VISION_WAV/GEMINI_AI.wav"); break;
          case 2: playWAV("/TACTI_VISION_WAV/NOTE_MAKER.wav"); break;
          case 3: playWAV("/TACTI_VISION_WAV/HID_SHORTCUT.wav"); break;
          case 4: playWAV("/TACTI_VISION_WAV/B_DRIVE.wav"); break;
          case 5: playWAV("/TACTI_VISION_WAV/SD_MODE.wav"); break;
          case 6: playWAV("/TACTI_VISION_WAV/SD_UPLOAD.wav"); break;
          case 7: playWAV("/TACTI_VISION_WAV/SYSTEM_UPDATE.wav"); break;
          case 8: playWAV("/TACTI_VISION_WAV/AUDIO_PLAYER.wav"); break;
        }
      } else if (currentAppMode == SD_NAVIGATION_MODE) {
        if (!sdFiles.empty() && currentFileIndex < sdFiles.size() - 1) {
          currentFileIndex++;
          Serial.print("Current: ");
          Serial.println(sdFiles[currentFileIndex]);
        }
      } else if (currentAppMode == HID_SHORTCUT_MODE_PLACEHOLDER) {
        if (selectedShortcutIndex < NUM_SHORTCUTS - 1) selectedShortcutIndex++;
        else selectedShortcutIndex = 0;
        Serial.print(">> ");
        Serial.println(shortcutNames[selectedShortcutIndex]);
        switch (selectedShortcutIndex) {
          case 0: playWAV("/TACTI_VISION_WAV/WIFI.wav"); break;
          case 1: playWAV("/TACTI_VISION_WAV/CHROME.wav"); break;
          case 2: playWAV("/TACTI_VISION_WAV/GMAIL.wav"); break;
          case 3: playWAV("/TACTI_VISION_WAV/CHATGPT.wav"); break;
          case 4: playWAV("/TACTI_VISION_WAV/PY_COMPILER.wav"); break;
          case 5: playWAV("/TACTI_VISION_WAV/LIBRARY.wav"); break;
        }
      }
      if (currentAppMode == AUDIO_PLAYER_MODE) {
        if (audioPlaying) {
          stopCurrentAudio();
        } else {
          nextAudioFile();
        }
      }
      lastDownPress = millis();
    }
  }

  lastDownState = currentDownState;
}

// Handle BACKSPACE button (Pin 7)
void handleBackspaceButton() {
  static unsigned long lastBackspacePress = 0;

  if (currentBackspaceState == LOW && lastBackspaceState == HIGH) {
    if (millis() - lastBackspacePress > DEBOUNCE_DELAY) {
      if (currentAppMode == PERKINS_MODE || currentAppMode == NOTE_MAKER_MODE || currentAppMode == GEMINI_AI_MODE_PLACEHOLDER || (currentAppMode == B_DRIVE_MODE && bdriveNaming) || (currentAppMode == GEMINI_AI_MODE_PLACEHOLDER && geminiNaming)) {

        if (!currentWord.empty()) {
          currentWord.pop_back();
          //Serial.print("\nBackspace: ");
          //Serial.println(currentWord.c_str());

          if (currentAppMode == PERKINS_MODE && hidKeyboardEnabled) {
            hidBackspace();
          }
        }
        vibe(0.1);
      }

      lastBackspacePress = millis();
    }
  }

  lastBackspaceState = currentBackspaceState;
}

// Handle SPACE buttons (Pins 8 and 9) - both behave identically (Left and Right Space(and also used as Modifiers) for comfort)
void handleSpaceButtons() {
  static unsigned long lastSpacePress = 0;

  // Either space button triggers the same action
  bool spacePressed = (currentSpaceLeftState == LOW || currentSpaceRightState == LOW);
  bool spaceWasPressed = (lastSpaceLeftState == LOW || lastSpaceRightState == LOW);

  if (spacePressed && !spaceWasPressed) {
    if (millis() - lastSpacePress > DEBOUNCE_DELAY) {
      if (currentAppMode == PERKINS_MODE || currentAppMode == NOTE_MAKER_MODE || currentAppMode == GEMINI_AI_MODE_PLACEHOLDER || (currentAppMode == B_DRIVE_MODE && bdriveNaming) || (currentAppMode == GEMINI_AI_MODE_PLACEHOLDER && geminiNaming)) {

        currentWord += " ";
        Serial.print(" ");

        if (currentAppMode == PERKINS_MODE && hidKeyboardEnabled) {
          hidSpace();
        }

        vibe(0.05);
      }

      lastSpacePress = millis();
    }
  }

  lastSpaceLeftState = currentSpaceLeftState;
  lastSpaceRightState = currentSpaceRightState;
}

// Handle CTRL button (Pin 6) - TOGGLE modifier key
void handleCtrlButton() {
  static bool ctrlToggled = false;
  static unsigned long lastCtrlPress = 0;

  if (currentCtrlState == LOW && lastCtrlState == HIGH) {
    if (millis() - lastCtrlPress > DEBOUNCE_DELAY) {
      // Ctrl button pressed - TOGGLE the state
      if (currentAppMode == PERKINS_MODE && hidKeyboardEnabled) {
        ctrlToggled = !ctrlToggled;

        if (ctrlToggled) {
          hidCtrlPress();
          Serial.println("CTRL TOGGLED ON");
          vibe(0.05);
        } else {
          hidCtrlRelease();
          Serial.println("CTRL TOGGLED OFF");
          vibe(0.05);
        }
      }

      lastCtrlPress = millis();
    }
  }

  lastCtrlState = currentCtrlState;
}

void loop() {
  checkDeepSleep();
  if (bleInitialized) {
    watch.loop();
  }
  processMPU6050Gestures();
  processNotificationRingBuffer();

  if (notifyActive) {
    if (millis() - notifyLastActivity > 60000) {
      notifyLastActivity = millis();
    }
  }

  if (currentAppMode == B_DRIVE_MODE) {
    bdriveServer.handleClient();
  }
  if (currentAppMode == SD_UPLOAD_MODE && sdUploadServerActive) {
    sdUploadServer.handleClient();
  }
  if (currentAppMode == SYSTEM_UPDATE_MODE && otaWaitingForConfirm) {
    if (millis() > otaConfirmTimeout) {
      Serial.println("[OTA] Timeout - update cancelled");
      cancelOTAUpdate();
    }
  }
  checkKeyCombinations();  // Check combos first

  if (millis() >= suppressSingleUntil) {
    handleSelectButton();
    handlePrevButton();
    handleUpButton();
    handleDownButton();
    handleBackspaceButton();
    handleSpaceButtons();
    handleCtrlButton();
  }

  // ================== Read from PCF8575 instead of GPIO ==================
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    updateSwitchStatesFromPCF();  // Read all switches from PCF8575
    lastDebounceTime = millis();
  }

  if ((millis() - lastBrailleCharTime) > BRAILLE_CHAR_DELAY) {
    int braillePattern = readBraillePattern();
    static int lastProcessedBraillePattern = 0;

    if (braillePattern != 0 && braillePattern != lastProcessedBraillePattern) {
      if (currentAppMode == PERKINS_MODE || currentAppMode == NOTE_MAKER_MODE || currentAppMode == GEMINI_AI_MODE_PLACEHOLDER || (currentAppMode == B_DRIVE_MODE && bdriveNaming) || (currentAppMode == GEMINI_AI_MODE_PLACEHOLDER && geminiNaming)) {
        processBrailleInput(braillePattern);
      } else if (currentAppMode == MODE_OPTIONS) {
        //handleModeOptionsNavigation();
      } else if (currentAppMode == SD_NAVIGATION_MODE) {
        //handleSdNavigation();
      }
      lastProcessedBraillePattern = braillePattern;
    } else if (braillePattern == 0) {
      lastProcessedBraillePattern = 0;
    }
    lastBrailleCharTime = millis();
  }
}

void populateBrailleMaps() {
  brailleMap[1] = 'a';
  brailleMap[3] = 'b';
  brailleMap[9] = 'c';
  brailleMap[25] = 'd';
  brailleMap[17] = 'e';
  brailleMap[11] = 'f';
  brailleMap[27] = 'g';
  brailleMap[19] = 'h';
  brailleMap[10] = 'i';
  brailleMap[26] = 'j';
  brailleMap[5] = 'k';
  brailleMap[7] = 'l';
  brailleMap[13] = 'm';
  brailleMap[29] = 'n';
  brailleMap[21] = 'o';
  brailleMap[15] = 'p';
  brailleMap[31] = 'q';
  brailleMap[23] = 'r';
  brailleMap[14] = 's';
  brailleMap[30] = 't';
  brailleMap[37] = 'u';
  brailleMap[39] = 'v';
  brailleMap[58] = 'w';
  brailleMap[45] = 'x';
  brailleMap[61] = 'y';
  brailleMap[53] = 'z';
  brailleMap[2] = ',';
  brailleMap[6] = ';';
  brailleMap[18] = ':';
  brailleMap[34] = '.';
  brailleMap[38] = '!';
  brailleMap[42] = '(';
  brailleMap[50] = ')';
  brailleMap[36] = '?';
  brailleMap[20] = '-';
  brailleMap[44] = '"';
  brailleMap[8] = '\'';
  brailleMap[48] = '=';

  numberMap[1] = '1';
  numberMap[3] = '2';
  numberMap[9] = '3';
  numberMap[25] = '4';
  numberMap[17] = '5';
  numberMap[11] = '6';
  numberMap[27] = '7';
  numberMap[19] = '8';
  numberMap[10] = '9';
  numberMap[26] = '0';
}




int readBraillePattern() {
  int pattern = 0;
  bool anyDotPressed = false;

  for (int i = 0; i < NUM_BRAILLE_PINS; i++) {
    if (currentBrailleStates[i] == LOW) {
      pattern |= (1 << i);
      anyDotPressed = true;
    }
  }
  return anyDotPressed ? pattern : 0;
}

void processBrailleInput(int braillePattern) {
  updateActivityTimer();
  char outputChar = ' ';

  if (braillePattern == NUMERIC_INDICATOR_PATTERN) {
    isNumberMode = true;
    playWAV("/TACTI_VISION_WAV/num.wav");
    vibe(0.2);
    Serial.print("[NUM]");
  } else if (braillePattern == CAPITAL_INDICATOR_PATTERN) {
    isCapitalMode = true;
    playWAV("/TACTI_VISION_WAV/caps.wav");
    vibe(0.2);
    Serial.print("[CAP]");
  } else {
    if (isNumberMode && numberMap.count(braillePattern)) {
      outputChar = numberMap[braillePattern];
      currentWord += outputChar;
      Serial.print(outputChar);

      if (currentAppMode == PERKINS_MODE && hidKeyboardEnabled) {
        hidPrintChar(outputChar);
      }

      isNumberMode = false;
    } else if (isCapitalMode && brailleMap.count(braillePattern)) {
      outputChar = brailleMap[braillePattern];
      if (outputChar >= 'a' && outputChar <= 'z') {
        outputChar = toupper(outputChar);
      }
      currentWord += outputChar;
      Serial.print(outputChar);

      // *** Play alphabet audio for capital letters ***
      if (alphabetAudioEnabled && outputChar >= 'A' && outputChar <= 'Z') {
        playAlphabetAudio(outputChar);
      }

      if (currentAppMode == PERKINS_MODE && hidKeyboardEnabled) {
        hidPrintChar(outputChar);
      }

      isCapitalMode = false;
    } else if (brailleMap.count(braillePattern)) {
      outputChar = brailleMap[braillePattern];
      currentWord += outputChar;
      Serial.print(outputChar);

      // *** Play alphabet audio for lowercase letters ***
      if (alphabetAudioEnabled && outputChar >= 'a' && outputChar <= 'z') {
        playAlphabetAudio(toupper(outputChar));
      }

      if (currentAppMode == PERKINS_MODE && hidKeyboardEnabled) {
        hidPrintChar(outputChar);
      }
    }

    if (isNumberMode && !numberMap.count(braillePattern)) {
      isNumberMode = false;
    }
    if (isCapitalMode && !brailleMap.count(braillePattern)) {
      isCapitalMode = false;
    }
  }
}

void enterMode(AppMode newMode) {
  updateActivityTimer();
  if (currentAppMode == newMode && newMode != BOOT_MODE) {
    return;
  }

  previousAppMode = currentAppMode;
  currentAppMode = newMode;
  currentWord = "";
  isNumberMode = false;
  isCapitalMode = false;
  isNaming = false;
  if (currentAppMode == PERKINS_MODE && newMode != PERKINS_MODE) {
    setHIDKeyboardMode(false);
  }
  if (newMode == NORMAL_MODE) {
    Serial.println();
    Serial.println("--- NORMAL MODE ---");
    vibe(0.2);
    //Serial.println("Press ENTER to go to PERKINS MODE, double tap ENTER for MODES.");
  } else if (newMode == PERKINS_MODE) {
    Serial.println();
    Serial.println("--- PERKINS MODE ---");
    playWAV("/TACTI_VISION_WAV/PERKINS_MODE.wav");
    vibe(0.2);

    // *** Enable HID keyboard for PERKINS mode ***
    setHIDKeyboardMode(true);
  } else if (newMode == MODE_OPTIONS) {
    Serial.println();
    Serial.println("--- MODE OPTIONS ---");
    playWAV("/TACTI_VISION_WAV/MODES.wav");
    vibe(0.2);
    selectedModeIndex = 0;
    Serial.print(">> ");
    Serial.println(modeNames[selectedModeIndex]);
  } else if (newMode == NOTE_MAKER_MODE) {
    Serial.println();
    Serial.println("--- NOTE-MAKER MODE ---");
    playWAV("/TACTI_VISION_WAV/NOTE_MAKER.wav");
    vibe(0.2);
    enterNoteMakerNaming();
  } else if (newMode == SD_NAVIGATION_MODE) {
    Serial.println();
    Serial.println("--- SD MODE ---");
    playWAV("/TACTI_VISION_WAV/SD_MODE.wav");
    vibe(0.2);

    if (sdCardAvailable) {
      listSdFiles("/", sdFiles);
      currentFileIndex = 0;
      if (sdFiles.empty()) {
        //Serial.println("No files found on SD card.");
      } else {
        Serial.print("Current file: ");
        Serial.println(sdFiles[currentFileIndex]);
        Serial.println("Press ctrl+backspace to get TTS Output");
      }
    } else {
      //Serial.println("SD card not available!");
    }
    // In enterMode function, replace the GEMINI_AI_MODE_PLACEHOLDER section with:
  } else if (newMode == GEMINI_AI_MODE_PLACEHOLDER) {
    Serial.println();
    Serial.println("--- GEMINI AI MODE ---");
    playWAV("/TACTI_VISION_WAV/GEMINI_AI.wav");
    vibe(0.2);
    playWAV("/TACTI_VISION_WAV/hold_query.wav");

    // Clear any previous state
    geminiResponseText = "";
    geminiWaitingForSave = false;
    geminiNaming = false;
    geminiFileName = "";

    ensureWiFiConnected();
  } else if (newMode == HID_SHORTCUT_MODE_PLACEHOLDER) {
    Serial.println();
    Serial.println("--- HID SHORTCUT MODE ---");
    playWAV("/TACTI_VISION_WAV/HID_SHORTCUT.wav");
    vibe(0.2);

    selectedShortcutIndex = 0;
    Serial.println("Available shortcuts:");
    for (int i = 0; i < NUM_SHORTCUTS; i++) {
      Serial.print(i == 0 ? ">> " : "   ");
      Serial.println(shortcutNames[i]);
    }
    Serial.println("Use UP/DOWN to navigate, SELECT to execute");
  } else if (newMode == B_DRIVE_MODE) {
    Serial.println();
    Serial.println("--- B-DRIVE MODE ---");
    playWAV("/TACTI_VISION_WAV/B_DRIVE.wav");
    vibe(0.2);

    bdriveExtractedText = "";
    bdriveTextReady = false;
    bdriveWaitingForSave = false;
    bdriveNaming = false;
    bdriveFileName = "";

    startBdriveServer();
  } else if (newMode == SD_UPLOAD_MODE) {
    Serial.println();
    Serial.println("--- SD UPLOAD MODE ---");
    playWAV("/TACTI_VISION_WAV/SD_MODE.wav");  // Reuse SD mode audio
    vibe(0.2);

    startSDUploadServer();
    Serial.println("[SD UPLOAD] Ready to receive files");
    playWAV("/TACTI_VISION_WAV/READY_TO_RECEIVE.wav");  // ← REPLACE EXISTING
    Serial.println("[SD UPLOAD] Press PREVIOUS to exit");
  }
  // === SYSTEM UPDATE MODE ===
  else if (newMode == SYSTEM_UPDATE_MODE) {
    Serial.println();
    Serial.println("--- SYSTEM UPDATE MODE ---");
    Serial.printf("[OTA] Current firmware: %s\n", CURRENT_FIRMWARE_VERSION);
    vibe(0.2);

    // Reset OTA state
    otaUpdateAvailable = false;
    otaWaitingForConfirm = false;
    otaNewVersion = "";
    otaFirmwareURL = "";

    // Start checking for updates
    checkForOTAUpdate();
  } else if (newMode == AUDIO_PLAYER_MODE) {
    Serial.println();
    Serial.println("--- AUDIO PLAYER MODE ---");
    playWAV("/TACTI_VISION_WAV/SELECTED.wav");  // Reuse existing audio
    vibe(0.2);

    audioPlayerActive = true;
    audioPlaying = false;
    playWAV("/TACTI_VISION_WAV/Audio_Init.wav");
    if (sdCardAvailable) {
      // Check if AudioFiles folder exists
      if (!SD.exists("/AudioFiles")) {
        Serial.println("[AUDIO PLAYER] Creating /AudioFiles folder...");
        SD.mkdir("/AudioFiles");
      }

      listAudioFiles("/AudioFiles", audioFiles);
      currentAudioIndex = 0;

      if (audioFiles.empty()) {
        Serial.println("[AUDIO PLAYER] No .wav files found in /AudioFiles folder");
        Serial.println("[AUDIO PLAYER] Please add audio files and restart mode");
      } else {
        Serial.print("[AUDIO PLAYER] Current: ");
        Serial.println(audioFiles[currentAudioIndex]);
        //Serial.println("[AUDIO PLAYER] UP/DOWN: Navigate | SELECT: Play | PREV: Exit");
      }
    } else {
      Serial.println("[AUDIO PLAYER] SD card not available!");
    }
  }
}

// Handle key combinations (replaces checkCombinedButtons)
void checkKeyCombinations() {
  bool spacePressed = (currentSpaceLeftState == LOW || currentSpaceRightState == LOW);
  bool ctrlPressed = (currentCtrlState == LOW);
  bool backspacePressed = (currentBackspaceState == LOW);

  // Check for BOTH space buttons pressed simultaneously for SOS
  bool bothSpacesPressed = (currentSpaceLeftState == LOW && currentSpaceRightState == LOW);

  unsigned long now = millis();
  static bool ctrlSpacePressed = false;
  static unsigned long ctrlSpaceComboStartTime = 0;

  // === SOS TRIGGER - SINGLE SPACE BUTTON LONG PRESS ===
  static bool leftSpaceLongPressActive = false;
  static bool rightSpaceLongPressActive = false;
  static unsigned long leftSpaceLongPressStart = 0;
  static unsigned long rightSpaceLongPressStart = 0;


  // Check left space button long press
  if (currentSpaceLeftState == LOW && lastSpaceLeftState == HIGH) {
    leftSpaceLongPressActive = true;
    leftSpaceLongPressStart = now;
  } else if (currentSpaceLeftState == HIGH && lastSpaceLeftState == LOW) {
    leftSpaceLongPressActive = false;
  }

  // Check right space button long press
  if (currentSpaceRightState == LOW && lastSpaceRightState == HIGH) {
    rightSpaceLongPressActive = true;
    rightSpaceLongPressStart = now;
  } else if (currentSpaceRightState == HIGH && lastSpaceRightState == LOW) {
    rightSpaceLongPressActive = false;
  }

  // Trigger SOS if either space button is held long enough
  // Trigger SOS if either space button is held long enough
  if (leftSpaceLongPressActive && (now - leftSpaceLongPressStart >= SOS_LONG_PRESS_DURATION)) {
    leftSpaceLongPressActive = false;
    suppressSingleUntil = now + SUPPRESS_SINGLE_MS;

    // Check cooldown (skip check if never used before)
    if (!sosInitialized || (now - lastSOSTime > SOS_COOLDOWN)) {
      Serial.println("\n[SOS] LEFT SPACE LONG PRESS = EMERGENCY ALERT!");

      if (sendSOSSMS()) {
        lastSOSTime = now;
        sosInitialized = true;  // Mark as used
        Serial.println("[SOS] ✅ Alert sent successfully!");
      } else {
        // Serial.println("[SOS] ❌ Alert failed to send - check WiFi/credentials");
      }
    } else {
      unsigned long remainingCooldown = (SOS_COOLDOWN - (now - lastSOSTime)) / 1000;
      Serial.printf("[SOS] Cooldown active - wait %lu seconds\n", remainingCooldown);
      vibe(0.1);
    }
    return;
  }

  if (rightSpaceLongPressActive && (now - rightSpaceLongPressStart >= SOS_LONG_PRESS_DURATION)) {
    rightSpaceLongPressActive = false;
    suppressSingleUntil = now + SUPPRESS_SINGLE_MS;

    // Check cooldown (skip check if never used before)
    if (!sosInitialized || (now - lastSOSTime > SOS_COOLDOWN)) {
      Serial.println("\n[SOS] RIGHT SPACE LONG PRESS = EMERGENCY ALERT!");

      if (sendSOSSMS()) {
        lastSOSTime = now;
        sosInitialized = true;  // Mark as used
        Serial.println("[SOS] ✅ Alert sent successfully!");
      } else {
        //Serial.println("[SOS] ❌ Alert failed to send - check WiFi/credentials");
      }
    } else {
      unsigned long remainingCooldown = (SOS_COOLDOWN - (now - lastSOSTime)) / 1000;
      Serial.printf("[SOS] Cooldown active - wait %lu seconds\n", remainingCooldown);
      vibe(0.1);
    }
    return;
  }

  // Continue with existing combo checks...
  if (ctrlPressed && spacePressed) {
    if (!ctrlSpacePressed) {
      ctrlSpacePressed = true;
      ctrlSpaceComboStartTime = now;
    }
    return;
  } else if (ctrlSpacePressed) {
    unsigned long dur = now - ctrlSpaceComboStartTime;
    ctrlSpacePressed = false;

    if (dur >= COMBO_HOLD_DURATION) {
      suppressSingleUntil = now + SUPPRESS_SINGLE_MS;
      Serial.println("[COMBO] Ctrl+Space = Text Correction");

      if (currentAppMode == PERKINS_MODE || currentAppMode == NOTE_MAKER_MODE || currentAppMode == GEMINI_AI_MODE_PLACEHOLDER || (currentAppMode == B_DRIVE_MODE && bdriveNaming) || (currentAppMode == GEMINI_AI_MODE_PLACEHOLDER && geminiNaming)) {
        applyTextCorrection();
      }
      vibe(0.15);
    }
  }

  // === COMBO: Ctrl + Backspace (6+7) for Save/Send/TTS ===
  static bool ctrlBackspacePressed = false;
  static unsigned long ctrlBackspaceComboStartTime = 0;

  if (ctrlPressed && backspacePressed) {
    if (!ctrlBackspacePressed) {
      ctrlBackspacePressed = true;
      ctrlBackspaceComboStartTime = now;
    }
    return;
  } else if (ctrlBackspacePressed) {
    unsigned long dur = now - ctrlBackspaceComboStartTime;
    ctrlBackspacePressed = false;

    if (dur >= COMBO_HOLD_DURATION) {
      suppressSingleUntil = now + SUPPRESS_SINGLE_MS;
      vibe(0.2);

      // === B-DRIVE: Save file ===
      if (currentAppMode == B_DRIVE_MODE && !bdriveNaming && bdriveFileName.length() > 0) {
        saveBdriveFile(bdriveFileName, bdriveExtractedText);
        bdriveExtractedText = "";
        bdriveTextReady = false;
        bdriveFileName = "";
        return;
      }

      // === GEMINI: Save response ===
      if (currentAppMode == GEMINI_AI_MODE_PLACEHOLDER && !geminiNaming && geminiFileName.length() > 0) {
        saveGeminiFile(geminiFileName, geminiResponseText);
        geminiResponseText = "";
        geminiWaitingForSave = false;
        geminiFileName = "";
        return;
      }

      // === NOTE MAKER: Save note ===
      if (currentAppMode == NOTE_MAKER_MODE) {
        if (newNoteFileName.length() == 0) {
          newNoteFileName = generateUniqueNoteName();
        }
        saveNoteToFile(newNoteFileName, String(currentWord.c_str()));
        lastSavedText = currentWord;
        currentWord = "";
        isNaming = false;
        playWAV("/TACTI_VISION_WAV/SAVED.wav");
        vibe(0.3);
        enterMode(MODE_OPTIONS);
        return;
      }

      // === SD NAVIGATION: TTS (chunked) ===
      if (currentAppMode == SD_NAVIGATION_MODE) {
        if (!sdFiles.empty() && sdCardAvailable) {
          String selectedFile = sdFiles[currentFileIndex];
          String fileContent = readFileContent(selectedFile.c_str());
          if (fileContent.length() == 0) {
            vibe(0.1);
            return;
          }
          playWAV("/TACTI_VISION_WAV/converting_TTS.wav");
          const int CHUNK_SIZE = 300;
          int totalChunks = (fileContent.length() + CHUNK_SIZE - 1) / CHUNK_SIZE;
          vibe(0.2);

          const char* ttsFile = "/TTS_temp.wav";
          int successfulChunks = 0;
          int failedChunks = 0;

          for (int i = 0; i < totalChunks; i++) {
            int startPos = i * CHUNK_SIZE;
            int endPos = min(startPos + CHUNK_SIZE, (int)fileContent.length());
            String chunk = fileContent.substring(startPos, endPos);

            if (i < totalChunks - 1) {
              int lastPeriod = chunk.lastIndexOf('.');
              int lastExclaim = chunk.lastIndexOf('!');
              int lastQuestion = chunk.lastIndexOf('?');
              int lastNewline = chunk.lastIndexOf('\n');
              int lastSentenceEnd = max(max(lastPeriod, lastExclaim), max(lastQuestion, lastNewline));

              if (lastSentenceEnd > CHUNK_SIZE / 2) {
                chunk = chunk.substring(0, lastSentenceEnd + 1);
              }
            }

            if (fetchTTSFromVoiceRSS(chunk.c_str(), ttsFile, true)) {
              delay(200);
              playWAV(ttsFile);
              delay(100);
              if (SD.exists(ttsFile)) {
                SD.remove(ttsFile);
              }
              successfulChunks++;
              if (i < totalChunks - 1) {
                delay(300);
              }
            } else {
              failedChunks++;
              vibe(0.08);
              delay(100);
            }
            yield();
          }

          if (failedChunks > 0) {
            vibe(0.15);
          } else {
            vibe(0.25);
          }
        }
        return;
      }

      // === GEMINI: Send query ===
      if (currentAppMode == GEMINI_AI_MODE_PLACEHOLDER) {
        if (!ensureWiFiConnected()) {
          Serial.println("WiFi not connected");
          vibe(0.2);
          return;
        }

        String query = String(currentWord.c_str());
        if (query.length() == 0) {
          Serial.println("Nothing to send");
          playWAV("/TACTI_VISION_WAV/hold_query.wav");
          vibe(0.08);
          return;
        }
        playWAV("/TACTI_VISION_WAV/Sending_Query.wav");
        String response = queryGemini(query);
        Serial.println("--- GEMINI RESPONSE ---");
        Serial.println(response);
        Serial.println("--- END ---");
        playWAV("/TACTI_VISION_WAV/response_received.wav");
        geminiResponseText = response;
        geminiWaitingForSave = true;
        lastSavedText = std::string(response.c_str());
        currentWord = "";

        Serial.println("[GEMINI AI] Response received. Press Select to save/Prev to skip");
        vibe(0.3);
        return;
      }
    }
  }

  // === COMBO: Space + Ctrl (8/9 + 6) for Shift+Enter ===
  static bool spaceCtrlPressed = false;
  static unsigned long spaceCtrlComboStartTime = 0;

  if (spacePressed && ctrlPressed) {
    if (!spaceCtrlPressed) {
      spaceCtrlPressed = true;
      spaceCtrlComboStartTime = now;
    }
    return;
  } else if (spaceCtrlPressed) {
    unsigned long dur = now - spaceCtrlComboStartTime;
    spaceCtrlPressed = false;

    if (dur >= COMBO_HOLD_DURATION) {
      suppressSingleUntil = now + SUPPRESS_SINGLE_MS;

      if (currentAppMode == PERKINS_MODE && hidKeyboardEnabled) {
        hidShiftEnter();
        currentWord += "\n";
      }
      vibe(0.15);
    }
  }

  // === COMBO: Space + Backspace (8/9 + 7) for Enter ===
  static bool spaceBackspacePressed = false;
  static unsigned long spaceBackspaceComboStartTime = 0;

  if (spacePressed && backspacePressed) {
    if (!spaceBackspacePressed) {
      spaceBackspacePressed = true;
      spaceBackspaceComboStartTime = now;
    }
    return;
  } else if (spaceBackspacePressed) {
    unsigned long dur = now - spaceBackspaceComboStartTime;
    spaceBackspacePressed = false;

    if (dur >= COMBO_HOLD_DURATION) {
      suppressSingleUntil = now + SUPPRESS_SINGLE_MS;

      if (currentAppMode == PERKINS_MODE && hidKeyboardEnabled) {
        hidEnter();
        currentWord += "\n";
      }
      vibe(0.15);
    }
  }

  // === SPECIAL: All 6 dots pressed = Toggle Alphabet Audio ===
  int currentPattern = readBraillePattern();
  static int lastPattern = 0;
  static unsigned long lastPatternTime = 0;

  if (currentPattern == 63 && lastPattern != 63) {
    if (now - lastPatternTime > 500) {
      toggleAlphabetAudio();
      lastPatternTime = now;
    }
  }
  lastPattern = currentPattern;
}  // End of checkCombinedButtons()

void listSdFiles(const char* dirname, std::vector<String>& files) {
  files.clear();
  if (!sdCardAvailable) return;

  Serial.printf("Listing: %s\n", dirname);
  File dir = SD.open(dirname);
  if (!dir || !dir.isDirectory()) {
    //Serial.println("Failed to open directory");
    if (dir) dir.close();
    return;
  }

  File file = dir.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String name = String(file.name());
      if (name.charAt(0) == '/') {
        name = name.substring(1);
      }
      files.push_back(name);
      Serial.printf(" FILE: %s\n", name.c_str());
    }
    file = dir.openNextFile();
  }
  dir.close();
  std::sort(files.begin(), files.end());
}

void enterNoteMakerNaming() {
  Serial.println("Type note name (max 12 chars)");
  Serial.println("Double tap ENTER to confirm");
  newNoteFileName = generateUniqueNoteName();
  Serial.print("Default: ");
  Serial.println(newNoteFileName);
  currentWord = "";
  isNaming = true;
}

String generateUniqueNoteName() {
  if (!sdCardAvailable) return String("NOTE1.txt");

  for (int i = 1; i < 1000; i++) {
    String filename = String("NOTE") + String(i) + String(".txt");
    if (!SD.exists(filename.c_str()) && !SD.exists((String("/") + filename).c_str())) {
      return filename;
    }
  }
  return String("UNKNOWN_NOTE.txt");
}

void saveNoteToFile(const String& filename, const String& content) {
  if (!sdCardAvailable) {
    //Serial.println("SD card not available");
    return;
  }

  String fn = normalizeSdPath(filename);
  File noteFile = SD.open(fn.c_str(), FILE_WRITE);
  if (!noteFile) {
    //Serial.print("Failed to open file for writing: ");
    //Serial.println(fn);
    return;
  }

  size_t written = noteFile.print(content);
  //Serial.println(content);
  noteFile.flush();
  noteFile.close();

  if (written > 0) {
    //Serial.print("Successfully saved ");
    //Serial.print(written);
    //Serial.print(" bytes to ");
    //Serial.println(fn);
    vibe(0.3);
  } else {
    //Serial.print("Write failed - 0 bytes written to ");
    //Serial.println(fn);
    vibe(0.1);
  }
}

String normalizeSdPath(const String& pathIn) {
  String p = pathIn;
  if (p.length() == 0) return String("/");
  if (p.charAt(0) == '/') return p;

  if (sdCardAvailable && SD.exists(p.c_str())) return p;
  return String("/") + p;
}

bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return true;

  Serial.println("Connecting to WiFi...");
  playWAV("/TACTI_VISION_WAV/Connecting_wifi.wav");
  WiFi.mode(WIFI_STA);
  WiFi.begin(gemini_ssid, gemini_password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    playWAV("/TACTI_VISION_WAV/wificonnected.wav");
    //Serial.print("IP: ");
    //Serial.println(WiFi.localIP());
    return true;
  } else {
    //Serial.println("WiFi failed");
    return false;
  }
}

String queryGemini(const String& userQuery) {
  HTTPClient http;

  String url = String(gemini_api_url) + String("?key=") + String(gemini_api_key);
  http.begin(url);
  http.setTimeout(20000);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<512> requestDoc;
  JsonArray contents = requestDoc.createNestedArray("contents");
  JsonObject content = contents.createNestedObject();
  JsonArray parts = content.createNestedArray("parts");
  JsonObject part = parts.createNestedObject();
  part["text"] = userQuery;

  String jsonBody;
  serializeJson(requestDoc, jsonBody);

  Serial.println("\nSending to Gemini...");
  int httpCode = http.POST(jsonBody);

  String response = "Error";
  if (httpCode > 0) {
    String payload = http.getString();
    //Serial.println("Response received");

    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      if (doc.containsKey("candidates") && doc["candidates"].size() > 0) {
        JsonObject candidate = doc["candidates"][0];
        if (candidate.containsKey("content")) {
          JsonObject content = candidate["content"];
          if (content.containsKey("parts") && content["parts"].size() > 0) {
            const char* text = content["parts"][0]["text"];
            if (text) {
              response = String(text);
            } else {
              response = "No text in response";
            }
          } else {
            response = "No parts in response";
          }
        } else {
          response = "No content in response";
        }
      } else {
        response = "No candidates in response";
      }
    } else {
      response = String("JSON error: ") + String(error.c_str());
    }
  } else {
    response = String("HTTP error: ") + String(httpCode);
  }

  http.end();
  return response;
}
