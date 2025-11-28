#include <TinyWireS.h>
#include <DigiKeyboard.h>

// ==================== Configuration ====================
#define SLAVE_ADDR 0x23
#define LED_PIN 1

// Keyboard command codes
#define CMD_PRINT_CHAR 0x01
#define CMD_BACKSPACE 0x02
#define CMD_ENTER 0x03
#define CMD_SPACE 0x04
#define CMD_CTRL 0x05
#define CMD_SHIFT_ENTER 0x06
#define CMD_CTRL_PRESS 0x07    // Toggle Ctrl ON
#define CMD_CTRL_RELEASE 0x08  // Toggle Ctrl OFF
#define KEY_ESC 0x29

// Shortcut command codes
#define CMD_OPEN_WIFI 0x10
#define CMD_OPEN_CHROME 0x11
#define CMD_OPEN_GMAIL 0x12
#define CMD_OPEN_CHATGPT 0x13
#define CMD_OPEN_PYCOMPILER 0x14
#define CMD_OPEN_LIBRARY 0x15

// Queue configuration
#define QUEUE_SIZE 16

// ==================== Data Structures ====================
struct KeyCommand {
  uint8_t cmd;
  uint8_t data;
};

volatile KeyCommand cmdQueue[QUEUE_SIZE];
volatile uint8_t queueHead = 0;
volatile uint8_t queueTail = 0;

bool ctrlHeld = false;
unsigned long ctrlPressTime = 0;
const unsigned long CTRL_TIMEOUT = 30000;  // 30 seconds timeout for safety

// ==================== I2C Callbacks ====================

void receiveEvent(uint8_t howMany) {
  if (howMany >= 3) {
    uint8_t magic1 = TinyWireS.read();
    uint8_t magic2 = TinyWireS.read();

    // Check for shortcut command magic bytes
    if (magic1 == 0x12 && magic2 == 0x34) {
      uint8_t cmd = TinyWireS.read();

      // Add to queue
      uint8_t nextHead = (queueHead + 1) % QUEUE_SIZE;
      if (nextHead != queueTail) {
        cmdQueue[queueHead].cmd = cmd;
        cmdQueue[queueHead].data = 0;
        queueHead = nextHead;
      }
    }

    // Discard any extra bytes
    while (howMany > 3) {
      TinyWireS.read();
      howMany--;
    }
  } else if (howMany >= 2) {
    // Regular keyboard commands
    uint8_t cmd = TinyWireS.read();
    uint8_t data = TinyWireS.read();

    uint8_t nextHead = (queueHead + 1) % QUEUE_SIZE;
    if (nextHead != queueTail) {
      cmdQueue[queueHead].cmd = cmd;
      cmdQueue[queueHead].data = data;
      queueHead = nextHead;
    }
  } else {
    while (howMany > 0) {
      TinyWireS.read();
      howMany--;
    }
  }
}

void requestEvent() {
  uint8_t status = (queueHead == queueTail) ? 0x00 : 0x01;
  TinyWireS.write(status);
}

// ==================== Shortcut Functions ====================

void openWiFiSettings() {
  digitalWrite(LED_PIN, HIGH);
  DigiKeyboard.delay(2000);

  // Windows + A (Open Action Center)
  DigiKeyboard.sendKeyStroke(KEY_A, MOD_GUI_LEFT);
  DigiKeyboard.delay(2500);

  // Press Enter (toggle WiFi)
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  DigiKeyboard.delay(1000);

  // Press Esc (close Action Center)
  DigiKeyboard.sendKeyStroke(KEY_ESC);
  DigiKeyboard.delay(500);

  digitalWrite(LED_PIN, LOW);
}

void openChrome() {
  digitalWrite(LED_PIN, HIGH);
  DigiKeyboard.delay(2000);

  // Windows + R (Run dialog)
  DigiKeyboard.sendKeyStroke(KEY_R, MOD_GUI_LEFT);
  DigiKeyboard.delay(1500);

  DigiKeyboard.print("chrome");
  DigiKeyboard.delay(500);

  // Press Enter
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  DigiKeyboard.delay(3000);

  digitalWrite(LED_PIN, LOW);
}

void openURL(const char* url) {
  digitalWrite(LED_PIN, HIGH);
  DigiKeyboard.delay(2000);

  // Windows + R (Run dialog)
  DigiKeyboard.sendKeyStroke(KEY_R, MOD_GUI_LEFT);
  DigiKeyboard.delay(1500);

  // Type URL
  DigiKeyboard.print(url);
  DigiKeyboard.delay(500);

  // Press Enter
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  DigiKeyboard.delay(3000);

  digitalWrite(LED_PIN, LOW);
}

void openGmail() {
  openURL("https://mail.google.com/");
}

void openChatGPT() {
  openURL("https://chatgpt.com/");
}

void openPyCompiler() {
  openURL("https://www.programiz.com/python-programming/online-compiler/");
}

void openLibrary() {
  openURL("https://openlibrary.org/");
}

// ==================== Command Processing ====================

void processCommands() {
  while (queueTail != queueHead) {
    uint8_t cmd_code = cmdQueue[queueTail].cmd;
    uint8_t cmd_data = cmdQueue[queueTail].data;
    queueTail = (queueTail + 1) % QUEUE_SIZE;

    digitalWrite(LED_PIN, HIGH);

    // ===== KEYBOARD COMMANDS =====
    if (cmd_code == CMD_PRINT_CHAR) {
      // If Ctrl is held, send key with Ctrl modifier
      if (ctrlHeld) {
        // For letters, send the keycode with Ctrl
        if (cmd_data >= 'a' && cmd_data <= 'z') {
          uint8_t keyCode = KEY_A + (cmd_data - 'a');
          DigiKeyboard.sendKeyStroke(keyCode, MOD_CONTROL_LEFT);
        } else if (cmd_data >= 'A' && cmd_data <= 'Z') {
          uint8_t keyCode = KEY_A + (cmd_data - 'A');
          DigiKeyboard.sendKeyStroke(keyCode, MOD_CONTROL_LEFT);
        } else {
          // For other characters, just type normally with Ctrl held
          DigiKeyboard.write(cmd_data);
        }
      } else {
        // Normal typing without Ctrl
        DigiKeyboard.write(cmd_data);
      }
      DigiKeyboard.delay(30);
    } else if (cmd_code == CMD_BACKSPACE) {
      DigiKeyboard.sendKeyStroke(42);  // Backspace keycode
      DigiKeyboard.delay(30);
    } else if (cmd_code == CMD_ENTER) {
      DigiKeyboard.sendKeyStroke(40);  // Enter keycode
      DigiKeyboard.delay(30);
    } else if (cmd_code == CMD_SPACE) {
      DigiKeyboard.write(' ');
      DigiKeyboard.delay(30);
    } else if (cmd_code == CMD_SHIFT_ENTER) {
      DigiKeyboard.sendKeyStroke(40, MOD_SHIFT_LEFT);
      DigiKeyboard.delay(30);
    }

    // ===== CTRL TOGGLE COMMANDS (FIXED!) =====
    else if (cmd_code == CMD_CTRL_PRESS) {
      if (!ctrlHeld) {
        // Turn ON Ctrl toggle
        ctrlHeld = true;
        ctrlPressTime = millis();

        // Visual feedback: quick LED pulse
        digitalWrite(LED_PIN, LOW);
        DigiKeyboard.delay(50);
        digitalWrite(LED_PIN, HIGH);
        DigiKeyboard.delay(50);

        // Keep Ctrl pressed by sending empty keypress with modifier
        DigiKeyboard.sendKeyPress(0, MOD_CONTROL_LEFT);
      }
      DigiKeyboard.delay(10);
    } else if (cmd_code == CMD_CTRL_RELEASE) {
      if (ctrlHeld) {
        // Turn OFF Ctrl toggle
        ctrlHeld = false;
        ctrlPressTime = 0;

        // Release all keys
        DigiKeyboard.sendKeyPress(0, 0);

        // Visual feedback: quick LED pulse
        digitalWrite(LED_PIN, LOW);
        DigiKeyboard.delay(50);
        digitalWrite(LED_PIN, HIGH);
        DigiKeyboard.delay(50);
      }
      DigiKeyboard.delay(10);
    }

    // ===== SHORTCUT COMMANDS =====
    else if (cmd_code == CMD_OPEN_WIFI) {
      openWiFiSettings();
    } else if (cmd_code == CMD_OPEN_CHROME) {
      openChrome();
    } else if (cmd_code == CMD_OPEN_GMAIL) {
      openGmail();
    } else if (cmd_code == CMD_OPEN_CHATGPT) {
      openChatGPT();
    } else if (cmd_code == CMD_OPEN_PYCOMPILER) {
      openPyCompiler();
    } else if (cmd_code == CMD_OPEN_LIBRARY) {
      openLibrary();
    }

    // Unknown command - blink error
    else {
      digitalWrite(LED_PIN, LOW);
      DigiKeyboard.delay(50);
      digitalWrite(LED_PIN, HIGH);
      DigiKeyboard.delay(50);
    }

    digitalWrite(LED_PIN, LOW);
    DigiKeyboard.delay(5);
  }
}

void checkCtrlTimeout() {
  if (ctrlHeld) {
    unsigned long elapsed = millis() - ctrlPressTime;

    // Safety timeout to prevent Ctrl from being stuck forever
    if (elapsed > CTRL_TIMEOUT || elapsed < 0) {
      DigiKeyboard.sendKeyPress(0, 0);  // Release all keys
      ctrlHeld = false;
      ctrlPressTime = 0;

      // Warning blinks: Ctrl auto-released
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        DigiKeyboard.delay(100);
        digitalWrite(LED_PIN, LOW);
        DigiKeyboard.delay(100);
      }
    }
  }
}

// ==================== Setup & Loop ====================

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  TinyWireS.begin(SLAVE_ADDR);
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);

  DigiKeyboard.delay(500);

  // Startup: 3 blinks
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    DigiKeyboard.delay(100);
    digitalWrite(LED_PIN, LOW);
    DigiKeyboard.delay(100);
  }

  ctrlHeld = false;
  ctrlPressTime = 0;
}

void loop() {
  TinyWireS_stop_check();
  processCommands();
  checkCtrlTimeout();

  // CRITICAL: Maintain Ctrl state if toggled ON
  if (ctrlHeld) {
    // Re-send Ctrl modifier every loop to keep it active
    DigiKeyboard.sendKeyPress(0, MOD_CONTROL_LEFT);
  }

  DigiKeyboard.delay(10);
}