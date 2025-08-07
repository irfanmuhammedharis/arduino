/*
 * ESP32 RFID Audio Recorder
 * 
 * This sketch implements an RFID-triggered audio recording system for ESP32.
 * Fixes implemented:
 * 1. Robust button debouncing and detection
 * 2. Stable RFID tag presence detection
 * 3. Improved state machine handling
 * 4. Better timing management and user feedback
 * 
 * Hardware Requirements:
 * - ESP32 Dev Board
 * - RC522 RFID Reader
 * - I2S Microphone (INMP441)
 * - SD Card Module
 * - Push Button
 * - LED for status indication
 * 
 * See HARDWARE_SETUP.md for detailed wiring instructions.
 */

#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>
#include <FS.h>
#include <driver/i2s.h>
#include "config.h"

// System states
enum SystemState {
  STATE_IDLE,
  STATE_RFID_DETECTED,
  STATE_WAITING_FOR_BUTTON,
  STATE_RECORDING,
  STATE_PLAYBACK,
  STATE_ERROR
};

// Global variables
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
SystemState currentState = STATE_IDLE;
String lastUID = "";
String currentUID = "";

// Button handling variables
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long buttonPressTime = 0;
bool buttonPressed = false;
bool longPressDetected = false;

// RFID handling variables
unsigned long lastRFIDDetectionTime = 0;
unsigned long rfidRemovalStartTime = 0;
bool rfidTagPresent = false;
bool rfidRemovalInProgress = false;
int rfidDetectionCount = 0;

// Audio recording variables
File audioFile;
bool recording = false;
uint32_t recordedSamples = 0;
uint8_t i2sData[AUDIO_BUFFER_SIZE];

// Status LED variables
unsigned long lastBlinkTime = 0;
bool ledState = false;
int blinkPattern = 0; // 0=off, 1=slow, 2=fast, 3=solid

// Performance monitoring
unsigned long lastHealthCheck = 0;
unsigned long systemStartTime = 0;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  
  systemStartTime = millis();
  Serial.println("ESP32 RFID Audio Recorder Starting...");
  Serial.println("Version: 1.0 - With Button and RFID Fixes");
  
  // Initialize pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // Initialize SPI
  SPI.begin();
  
  // Initialize RFID with retry
  int rfidRetries = 0;
  while (rfidRetries < MAX_INIT_RETRIES) {
    rfid.PCD_Init();
    if (rfid.PCD_PerformSelfTest()) {
      Serial.println("RFID Reader initialized successfully");
      break;
    } else {
      rfidRetries++;
      Serial.println("RFID initialization failed, retrying...");
      delay(1000);
    }
  }
  
  if (rfidRetries >= MAX_INIT_RETRIES) {
    Serial.println("RFID initialization failed after retries");
    currentState = STATE_ERROR;
    setLEDPattern(2); // Fast blink for error
    return;
  }
  
  // Initialize SD card with retry
  int sdRetries = 0;
  while (sdRetries < MAX_INIT_RETRIES) {
    if (SD.begin(SD_CS_PIN)) {
      Serial.println("SD Card initialized successfully");
      break;
    } else {
      sdRetries++;
      Serial.println("SD Card initialization failed, retrying...");
      delay(SD_INIT_RETRY_DELAY);
    }
  }
  
  if (sdRetries >= MAX_INIT_RETRIES) {
    Serial.println("SD Card initialization failed after retries");
    currentState = STATE_ERROR;
    setLEDPattern(2); // Fast blink for error
    return;
  }
  
  // Initialize I2S for audio recording
  if (!initializeI2S()) {
    Serial.println("I2S initialization failed");
    currentState = STATE_ERROR;
    setLEDPattern(2); // Fast blink for error
    return;
  }
  
  // Show ready status
  setLEDPattern(1); // Slow blink for ready
  currentState = STATE_IDLE;
  lastHealthCheck = millis();
  
  Serial.println("System ready. Place RFID tag to start.");
  Serial.println("Type 'help' for available commands.");
}

void loop() {
  updateButton();
  updateRFID();
  updateStateMachine();
  updateStatusLED();
  
  // Handle serial commands for debugging
  if (Serial.available()) {
    handleSerialCommand();
  }
  
  // Perform periodic system health checks
  performSystemHealthCheck();
  
  delay(10); // Small delay for stability
}

void updateButton() {
  // Read button with debouncing
  int reading = digitalRead(BUTTON_PIN);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_DELAY) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
      
      if (currentButtonState == LOW) {
        // Button pressed
        buttonPressTime = millis();
        buttonPressed = true;
        longPressDetected = false;
        Serial.println("Button pressed");
      } else {
        // Button released
        if (buttonPressed) {
          unsigned long pressDuration = millis() - buttonPressTime;
          if (pressDuration >= BUTTON_LONG_PRESS_TIME) {
            longPressDetected = true;
            Serial.println("Long button press detected");
          } else {
            Serial.println("Short button press detected");
          }
          buttonPressed = false;
        }
      }
    }
  }
  
  // Check for long press while button is still held
  if (currentButtonState == LOW && buttonPressed && !longPressDetected) {
    if ((millis() - buttonPressTime) >= BUTTON_LONG_PRESS_TIME) {
      longPressDetected = true;
      Serial.println("Long button press detected (while held)");
    }
  }
  
  lastButtonState = reading;
}

void updateRFID() {
  bool tagDetected = false;
  String detectedUID = "";
  
  // Check for new cards with retry mechanism
  for (int attempt = 0; attempt < RFID_RETRY_ATTEMPTS; attempt++) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      detectedUID = getUID();
      tagDetected = true;
      break;
    }
    delay(10); // Small delay between attempts
  }
  
  unsigned long currentTime = millis();
  
  if (tagDetected) {
    // Tag detected
    if (!rfidTagPresent || detectedUID != currentUID) {
      // New tag or first detection
      currentUID = detectedUID;
      rfidTagPresent = true;
      rfidRemovalInProgress = false;
      lastRFIDDetectionTime = currentTime;
      rfidDetectionCount = 1;
      Serial.println("RFID Tag detected: " + currentUID);
    } else {
      // Same tag still present
      lastRFIDDetectionTime = currentTime;
      rfidDetectionCount++;
      if (rfidRemovalInProgress) {
        Serial.println("RFID Tag still present - canceling removal");
        rfidRemovalInProgress = false;
      }
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  } else {
    // No tag detected
    if (rfidTagPresent) {
      // Tag was present but not detected now
      if (!rfidRemovalInProgress) {
        // Start removal timer
        rfidRemovalStartTime = currentTime;
        rfidRemovalInProgress = true;
        Serial.println("RFID Tag removal detected - starting verification...");
      } else {
        // Check if removal delay has passed
        if ((currentTime - rfidRemovalStartTime) >= RFID_REMOVAL_DELAY) {
          // Confirm tag removal
          rfidTagPresent = false;
          rfidRemovalInProgress = false;
          currentUID = "";
          Serial.println("RFID Tag removed - confirmed");
        }
      }
    }
  }
}

void updateStateMachine() {
  switch (currentState) {
    case STATE_IDLE:
      if (rfidTagPresent && !rfidRemovalInProgress) {
        currentState = STATE_RFID_DETECTED;
        setLEDPattern(3); // Solid LED
        Serial.println("State: RFID Detected");
      }
      break;
      
    case STATE_RFID_DETECTED:
      if (!rfidTagPresent) {
        currentState = STATE_IDLE;
        setLEDPattern(1); // Slow blink
        Serial.println("State: Idle (RFID removed)");
      } else {
        currentState = STATE_WAITING_FOR_BUTTON;
        Serial.println("State: Waiting for button press");
      }
      break;
      
    case STATE_WAITING_FOR_BUTTON:
      if (!rfidTagPresent) {
        currentState = STATE_IDLE;
        setLEDPattern(1); // Slow blink
        Serial.println("State: Idle (RFID removed)");
      } else if (buttonPressed && currentButtonState == LOW) {
        // Button is currently being pressed
        if (longPressDetected) {
          // Long press - start playback
          startPlayback();
        } else {
          // Short press - start recording
          startRecording();
        }
      }
      break;
      
    case STATE_RECORDING:
      if (!rfidTagPresent) {
        stopRecording();
        currentState = STATE_IDLE;
        setLEDPattern(1); // Slow blink
        Serial.println("Recording stopped - RFID removed");
      } else if (buttonPressed && currentButtonState == HIGH) {
        // Button released - stop recording
        stopRecording();
        currentState = STATE_WAITING_FOR_BUTTON;
        setLEDPattern(3); // Solid LED
        Serial.println("Recording stopped - button released");
      }
      break;
      
    case STATE_PLAYBACK:
      if (!rfidTagPresent) {
        stopPlayback();
        currentState = STATE_IDLE;
        setLEDPattern(1); // Slow blink
        Serial.println("Playback stopped - RFID removed");
      } else if (buttonPressed && currentButtonState == HIGH) {
        // Button released - stop playback
        stopPlayback();
        currentState = STATE_WAITING_FOR_BUTTON;
        setLEDPattern(3); // Solid LED
        Serial.println("Playback stopped - button released");
      }
      break;
      
    case STATE_ERROR:
      // Error state - blink fast and try to recover
      if (millis() % 5000 == 0) {
        Serial.println("Attempting to recover from error state...");
        // Try to reinitialize components
        if (SD.begin(SD_CS_PIN)) {
          currentState = STATE_IDLE;
          setLEDPattern(1); // Slow blink for ready
          Serial.println("Recovered from error state");
        }
      }
      break;
  }
}

void startRecording() {
  if (!rfidTagPresent) {
    Serial.println("Cannot start recording - no RFID tag present");
    return;
  }
  
  String filename = "/" + String(AUDIO_FILE_PREFIX) + currentUID + String(AUDIO_FILE_EXTENSION);
  
  // Check if file already exists and create unique name if needed
  int fileCounter = 1;
  String originalFilename = filename;
  while (SD.exists(filename) && fileCounter < 100) {
    filename = "/" + String(AUDIO_FILE_PREFIX) + currentUID + "_" + String(fileCounter) + String(AUDIO_FILE_EXTENSION);
    fileCounter++;
  }
  
  audioFile = SD.open(filename, FILE_WRITE);
  
  if (!audioFile) {
    Serial.println("Failed to create audio file: " + filename);
    currentState = STATE_ERROR;
    setLEDPattern(2); // Fast blink for error
    return;
  }
  
  // Write WAV header (will be updated later with actual size)
  writeWAVHeader();
  
  recording = true;
  recordedSamples = 0;
  currentState = STATE_RECORDING;
  setLEDPattern(2); // Fast blink for recording
  
  Serial.println("Recording started: " + filename);
  
  if (DEBUG_LEVEL >= DEBUG_LEVEL_INFO) {
    Serial.println("Free SD space: " + String((SD.totalBytes() - SD.usedBytes()) / 1024) + " KB");
  }
}

void stopRecording() {
  if (!recording) return;
  
  recording = false;
  
  // Update WAV header with actual file size
  updateWAVHeader();
  audioFile.close();
  
  Serial.println("Recording stopped. Samples recorded: " + String(recordedSamples));
}

void startPlayback() {
  if (!rfidTagPresent) {
    Serial.println("Cannot start playback - no RFID tag present");
    return;
  }
  
  String filename = "/" + String(AUDIO_FILE_PREFIX) + currentUID + String(AUDIO_FILE_EXTENSION);
  
  // If the exact filename doesn't exist, look for numbered variants
  if (!SD.exists(filename)) {
    bool found = false;
    for (int i = 1; i < 100; i++) {
      String numberedFilename = "/" + String(AUDIO_FILE_PREFIX) + currentUID + "_" + String(i) + String(AUDIO_FILE_EXTENSION);
      if (SD.exists(numberedFilename)) {
        filename = numberedFilename;
        found = true;
        break;
      }
    }
    
    if (!found) {
      Serial.println("No audio file found for this RFID tag: " + currentUID);
      // Provide visual feedback
      for (int i = 0; i < 6; i++) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(100);
        digitalWrite(STATUS_LED_PIN, LOW);
        delay(100);
      }
      return;
    }
  }
  
  currentState = STATE_PLAYBACK;
  setLEDPattern(1); // Slow blink for playback
  
  Serial.println("Playback started: " + filename);
  
  // TODO: Implement actual audio playback using I2S
  // For now, just simulate playback duration
  delay(1000); // Simulate some playback time
}

void stopPlayback() {
  Serial.println("Playback stopped");
  // TODO: Implement actual playback stop
}

String getUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

bool initializeI2S() {
  i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = DMA_BUF_COUNT,
    .dma_buf_len = DMA_BUF_LEN
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  
  esp_err_t result = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    Serial.println("Failed to install I2S driver");
    return false;
  }
  
  result = i2s_set_pin(I2S_PORT, &pin_config);
  if (result != ESP_OK) {
    Serial.println("Failed to set I2S pins");
    return false;
  }
  
  Serial.println("I2S initialized for audio recording");
  return true;
}

void writeWAVHeader() {
  // WAV file header
  audioFile.write((uint8_t*)"RIFF", 4);
  audioFile.write((uint8_t*)"\x00\x00\x00\x00", 4); // File size (will be updated)
  audioFile.write((uint8_t*)"WAVE", 4);
  audioFile.write((uint8_t*)"fmt ", 4);
  audioFile.write((uint8_t*)"\x10\x00\x00\x00", 4); // Subchunk1Size
  audioFile.write((uint8_t*)"\x01\x00", 2); // AudioFormat (PCM)
  audioFile.write((uint8_t*)"\x01\x00", 2); // NumChannels
  
  uint32_t sampleRate = SAMPLE_RATE;
  audioFile.write((uint8_t*)&sampleRate, 4);
  
  uint32_t byteRate = SAMPLE_RATE * 2; // 16-bit mono
  audioFile.write((uint8_t*)&byteRate, 4);
  
  audioFile.write((uint8_t*)"\x02\x00", 2); // BlockAlign
  audioFile.write((uint8_t*)"\x10\x00", 2); // BitsPerSample
  audioFile.write((uint8_t*)"data", 4);
  audioFile.write((uint8_t*)"\x00\x00\x00\x00", 4); // Subchunk2Size (will be updated)
}

void updateWAVHeader() {
  uint32_t fileSize = audioFile.size() - 8;
  uint32_t dataSize = audioFile.size() - WAVE_HEADER_SIZE;
  
  audioFile.seek(4);
  audioFile.write((uint8_t*)&fileSize, 4);
  
  audioFile.seek(40);
  audioFile.write((uint8_t*)&dataSize, 4);
}

void setLEDPattern(int pattern) {
  blinkPattern = pattern;
}

void updateStatusLED() {
  unsigned long currentTime = millis();
  
  switch (blinkPattern) {
    case 0: // Off
      digitalWrite(STATUS_LED_PIN, LOW);
      break;
      
    case 1: // Slow blink
      if (currentTime - lastBlinkTime >= LED_SLOW_BLINK_PERIOD / 2) {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState);
        lastBlinkTime = currentTime;
      }
      break;
      
    case 2: // Fast blink
      if (currentTime - lastBlinkTime >= LED_FAST_BLINK_PERIOD / 2) {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState);
        lastBlinkTime = currentTime;
      }
      break;
      
    case 3: // Solid on
      digitalWrite(STATUS_LED_PIN, HIGH);
      break;
  }
}

void handleSerialCommand() {
  String command = Serial.readStringUntil('\n');
  command.trim();
  
  if (command == "status") {
    printSystemStatus();
  } else if (command == "reset") {
    Serial.println("Resetting system...");
    ESP.restart();
  } else if (command == "files") {
    listAudioFiles();
  } else if (command.startsWith("delete ")) {
    String filename = command.substring(7);
    deleteAudioFile(filename);
  } else if (command == "help") {
    printHelp();
  } else {
    Serial.println("Unknown command: " + command);
    Serial.println("Type 'help' for available commands");
  }
}

void printSystemStatus() {
  Serial.println("=== System Status ===");
  Serial.println("State: " + String(getStateName()));
  Serial.println("RFID Tag Present: " + String(rfidTagPresent ? "Yes" : "No"));
  Serial.println("Current UID: " + currentUID);
  Serial.println("Button State: " + String(currentButtonState == LOW ? "Pressed" : "Released"));
  Serial.println("Recording: " + String(recording ? "Yes" : "No"));
  Serial.println("Free SD Space: " + String(SD.totalBytes() - SD.usedBytes()) + " bytes");
  Serial.println("=====================");
}

String getStateName() {
  switch (currentState) {
    case STATE_IDLE: return "Idle";
    case STATE_RFID_DETECTED: return "RFID Detected";
    case STATE_WAITING_FOR_BUTTON: return "Waiting for Button";
    case STATE_RECORDING: return "Recording";
    case STATE_PLAYBACK: return "Playback";
    case STATE_ERROR: return "Error";
    default: return "Unknown";
  }
}

void listAudioFiles() {
  Serial.println("=== Audio Files ===");
  File root = SD.open("/");
  File file = root.openNextFile();
  
  while (file) {
    if (!file.isDirectory() && String(file.name()).endsWith(".wav")) {
      Serial.println(String(file.name()) + " (" + String(file.size()) + " bytes)");
    }
    file = root.openNextFile();
  }
  Serial.println("==================");
}

void deleteAudioFile(String filename) {
  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }
  
  if (SD.exists(filename)) {
    if (SD.remove(filename)) {
      Serial.println("File deleted: " + filename);
    } else {
      Serial.println("Failed to delete file: " + filename);
    }
  } else {
    Serial.println("File not found: " + filename);
  }
}

void printHelp() {
  Serial.println("=== Available Commands ===");
  Serial.println("status - Show system status");
  Serial.println("files - List audio files");
  Serial.println("delete <filename> - Delete audio file");
  Serial.println("reset - Reset the system");
  Serial.println("help - Show this help");
  Serial.println("==========================");
}

void performSystemHealthCheck() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastHealthCheck >= SYSTEM_HEALTH_CHECK) {
    lastHealthCheck = currentTime;
    
    // Check system uptime
    unsigned long uptime = currentTime - systemStartTime;
    
    // Check free heap memory
    size_t freeHeap = ESP.getFreeHeap();
    
    // Log health status if debug enabled
    if (DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE) {
      Serial.println("=== System Health Check ===");
      Serial.println("Uptime: " + String(uptime / 1000) + " seconds");
      Serial.println("Free heap: " + String(freeHeap) + " bytes");
      Serial.println("Current state: " + getStateName());
      Serial.println("==========================");
    }
    
    // Check for memory leaks (basic check)
    static size_t lastFreeHeap = freeHeap;
    if (freeHeap < lastFreeHeap * 0.8) {  // More than 20% memory loss
      Serial.println("WARNING: Possible memory leak detected");
    }
    lastFreeHeap = freeHeap;
    
    // Check if system is stuck in error state
    if (currentState == STATE_ERROR && uptime > ERROR_RECOVERY_INTERVAL) {
      Serial.println("Attempting automatic error recovery...");
      // Try to recover by reinitializing components
      if (SD.begin(SD_CS_PIN) && initializeI2S()) {
        currentState = STATE_IDLE;
        setLEDPattern(1);
        Serial.println("System recovered from error state");
      }
    }
  }
}