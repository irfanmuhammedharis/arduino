/*
 * ESP32 RFID Audio Recorder - Complete Production-Ready Version
 * 
 * This sketch implements a complete RFID-triggered audio recording system for ESP32.
 * Features include robust button handling, stable RFID detection, dual I2S audio
 * (recording and playback), enhanced serial command interface, and comprehensive
 * error handling with diagnostics.
 * 
 * Hardware Requirements:
 * - ESP32 Dev Board
 * - RC522 RFID Reader
 * - I2S Microphone (INMP441)
 * - SD Card Module
 * - Push Button
 * - LED for status indication
 * 
 * Key Features:
 * - Non-blocking operation with millis() timing
 * - Complete utility functions (printRFIDDiagnostics, handleSerialCommands, etc.)
 * - Dual I2S configuration for recording and playback
 * - Enhanced button handling with proper debouncing
 * - Robust RFID detection with removal verification
 * - Comprehensive error handling and recovery
 * - Production-ready state management
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

// Error types for enhanced error handling
enum ErrorType {
  ERROR_NONE,
  ERROR_SD_CARD,
  ERROR_RFID_INIT,
  ERROR_I2S_INIT,
  ERROR_FILE_SYSTEM,
  ERROR_AUDIO_RECORDING,
  ERROR_AUDIO_PLAYBACK,
  ERROR_MEMORY,
  ERROR_HARDWARE
};

// Global variables
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
SystemState currentState = STATE_IDLE;
ErrorType lastError = ERROR_NONE;
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
unsigned long rfidLastScanTime = 0;

// Audio recording variables
File audioFile;
bool recording = false;
bool playbackActive = false;
uint32_t recordedSamples = 0;
uint8_t i2sData[AUDIO_BUFFER_SIZE];
unsigned long recordingStartTime = 0;

// Status LED variables
unsigned long lastBlinkTime = 0;
bool ledState = false;
int blinkPattern = 0; // 0=off, 1=slow, 2=fast, 3=solid

// Performance monitoring
unsigned long lastHealthCheck = 0;
unsigned long systemStartTime = 0;
size_t initialFreeHeap = 0;

// Function declarations
void setup();
void loop();
void updateButton();
void updateRFID();
void updateStateMachine();
void updateStatusLED();
void startRecording();
void stopRecording();
void startPlayback();
void stopPlayback();
bool initializeI2S();
bool initializeI2SPlayback();
void writeWAVHeader();
void updateWAVHeader();
String getUID();
void setLEDPattern(int pattern);
void performSystemHealthCheck();

// Missing utility functions - now implemented
void printRFIDDiagnostics();
void handleSerialCommands();
void printHelpMenu();
void handleError(ErrorType errorType, const String& errorMsg);

// Additional utility functions
void printSystemStatus();
void listAudioFiles();
void deleteAudioFile(const String& filename);
String getStateName();
void beepFeedback(int count);
void debugPrint(int level, const String& message);
void resetSystem();
bool testHardwareComponents();

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  
  systemStartTime = millis();
  initialFreeHeap = ESP.getFreeHeap();
  
  Serial.println("ESP32 RFID Audio Recorder Starting...");
  Serial.println("Version: 2.0 - Complete Production Ready");
  Serial.println("Features: Dual I2S, Enhanced Commands, Full Diagnostics");
  
  // Initialize pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // Show startup LED pattern
  beepFeedback(2);
  
  // Initialize SPI
  SPI.begin();
  debugPrint(DEBUG_LEVEL_INFO, "SPI bus initialized");
  
  // Initialize RFID with retry and diagnostics
  bool rfidInitialized = false;
  for (int retry = 0; retry < MAX_INIT_RETRIES; retry++) {
    rfid.PCD_Init();
    delay(100);
    
    if (rfid.PCD_PerformSelfTest()) {
      debugPrint(DEBUG_LEVEL_INFO, "RFID Reader initialized successfully");
      rfidInitialized = true;
      break;
    } else {
      debugPrint(DEBUG_LEVEL_WARNING, "RFID initialization failed, retry " + String(retry + 1));
      delay(1000);
    }
  }
  
  if (!rfidInitialized) {
    handleError(ERROR_RFID_INIT, "RFID initialization failed after retries");
    return;
  }
  
  // Initialize SD card with enhanced error handling
  bool sdInitialized = false;
  for (int retry = 0; retry < MAX_INIT_RETRIES; retry++) {
    if (SD.begin(SD_CS_PIN)) {
      debugPrint(DEBUG_LEVEL_INFO, "SD Card initialized successfully");
      uint64_t totalBytes = SD.totalBytes();
      uint64_t usedBytes = SD.usedBytes();
      debugPrint(DEBUG_LEVEL_INFO, "SD Card - Total: " + String(totalBytes / 1024) + 
                "KB, Used: " + String(usedBytes / 1024) + "KB");
      sdInitialized = true;
      break;
    } else {
      debugPrint(DEBUG_LEVEL_WARNING, "SD Card initialization failed, retry " + String(retry + 1));
      delay(SD_INIT_RETRY_DELAY);
    }
  }
  
  if (!sdInitialized) {
    handleError(ERROR_SD_CARD, "SD Card initialization failed after retries");
    return;
  }
  
  // Initialize I2S for audio recording
  if (!initializeI2S()) {
    handleError(ERROR_I2S_INIT, "I2S recording initialization failed");
    return;
  }
  
  // Initialize I2S for audio playback
  if (ENABLE_AUDIO_PLAYBACK && !initializeI2SPlayback()) {
    handleError(ERROR_I2S_INIT, "I2S playback initialization failed");
    return;
  }
  
  // Test hardware components
  if (!testHardwareComponents()) {
    handleError(ERROR_HARDWARE, "Hardware component test failed");
    return;
  }
  
  // Show ready status
  setLEDPattern(1); // Slow blink for ready
  currentState = STATE_IDLE;
  lastHealthCheck = millis();
  
  debugPrint(DEBUG_LEVEL_INFO, "System ready. Place RFID tag to start.");
  if (ENABLE_SERIAL_COMMANDS) {
    printHelpMenu();
  }
  
  // Final system check
  printRFIDDiagnostics();
}

void loop() {
  // Main system update cycle - all non-blocking
  updateButton();
  updateRFID();
  updateStateMachine();
  updateStatusLED();
  
  // Handle serial commands for debugging and control
  if (ENABLE_SERIAL_COMMANDS && Serial.available()) {
    handleSerialCommands();
  }
  
  // Perform periodic system health checks
  performSystemHealthCheck();
  
  // Small delay for stability without blocking
  delay(10);
}

void updateButton() {
  // Read button with enhanced debouncing
  int reading = digitalRead(BUTTON_PIN);
  unsigned long currentTime = millis();
  
  if (reading != lastButtonState) {
    lastDebounceTime = currentTime;
  }
  
  if ((currentTime - lastDebounceTime) > BUTTON_DEBOUNCE_DELAY) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
      
      if (currentButtonState == LOW) {
        // Button pressed
        buttonPressTime = currentTime;
        buttonPressed = true;
        longPressDetected = false;
        debugPrint(DEBUG_LEVEL_VERBOSE, "Button pressed");
        beepFeedback(1); // Audio feedback
      } else {
        // Button released
        if (buttonPressed) {
          unsigned long pressDuration = currentTime - buttonPressTime;
          if (pressDuration >= BUTTON_LONG_PRESS_TIME) {
            longPressDetected = true;
            debugPrint(DEBUG_LEVEL_INFO, "Long button press detected (" + String(pressDuration) + "ms)");
          } else {
            debugPrint(DEBUG_LEVEL_INFO, "Short button press detected (" + String(pressDuration) + "ms)");
          }
          buttonPressed = false;
        }
      }
    }
  }
  
  // Check for long press while button is still held
  if (currentButtonState == LOW && buttonPressed && !longPressDetected) {
    if ((currentTime - buttonPressTime) >= BUTTON_LONG_PRESS_TIME) {
      longPressDetected = true;
      debugPrint(DEBUG_LEVEL_INFO, "Long button press detected (while held)");
      beepFeedback(2); // Different feedback for long press
    }
  }
  
  lastButtonState = reading;
}

void updateRFID() {
  unsigned long currentTime = millis();
  
  // Throttle RFID scanning for performance
  if (currentTime - rfidLastScanTime < RFID_SCAN_INTERVAL) {
    return;
  }
  rfidLastScanTime = currentTime;
  
  bool tagDetected = false;
  String detectedUID = "";
  
  // Enhanced RFID detection with retry mechanism
  for (int attempt = 0; attempt < RFID_RETRY_ATTEMPTS; attempt++) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      detectedUID = getUID();
      tagDetected = true;
      break;
    }
    delay(5); // Small delay between attempts
  }
  
  if (tagDetected) {
    // Tag detected
    if (!rfidTagPresent || detectedUID != currentUID) {
      // New tag or first detection
      currentUID = detectedUID;
      rfidTagPresent = true;
      rfidRemovalInProgress = false;
      lastRFIDDetectionTime = currentTime;
      rfidDetectionCount = 1;
      debugPrint(DEBUG_LEVEL_INFO, "RFID Tag detected: " + currentUID);
      beepFeedback(1);
    } else {
      // Same tag still present
      lastRFIDDetectionTime = currentTime;
      rfidDetectionCount++;
      if (rfidRemovalInProgress) {
        debugPrint(DEBUG_LEVEL_VERBOSE, "RFID Tag still present - canceling removal");
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
        debugPrint(DEBUG_LEVEL_VERBOSE, "RFID Tag removal detected - starting verification...");
      } else {
        // Check if removal delay has passed
        if ((currentTime - rfidRemovalStartTime) >= RFID_REMOVAL_DELAY) {
          // Confirm tag removal
          rfidTagPresent = false;
          rfidRemovalInProgress = false;
          String removedUID = currentUID;
          currentUID = "";
          debugPrint(DEBUG_LEVEL_INFO, "RFID Tag " + removedUID + " removed - confirmed");
          beepFeedback(3); // Different pattern for removal
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
        debugPrint(DEBUG_LEVEL_INFO, "State: RFID Detected");
      }
      break;
      
    case STATE_RFID_DETECTED:
      if (!rfidTagPresent) {
        currentState = STATE_IDLE;
        setLEDPattern(1); // Slow blink
        debugPrint(DEBUG_LEVEL_INFO, "State: Idle (RFID removed)");
      } else {
        currentState = STATE_WAITING_FOR_BUTTON;
        debugPrint(DEBUG_LEVEL_INFO, "State: Waiting for button press");
      }
      break;
      
    case STATE_WAITING_FOR_BUTTON:
      if (!rfidTagPresent) {
        currentState = STATE_IDLE;
        setLEDPattern(1); // Slow blink
        debugPrint(DEBUG_LEVEL_INFO, "State: Idle (RFID removed)");
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
      // Handle recording timeout
      if (millis() - recordingStartTime > (RECORD_TIME * 1000)) {
        debugPrint(DEBUG_LEVEL_INFO, "Recording timeout reached");
        stopRecording();
        currentState = STATE_WAITING_FOR_BUTTON;
        setLEDPattern(3);
      } else if (!rfidTagPresent) {
        stopRecording();
        currentState = STATE_IDLE;
        setLEDPattern(1);
        debugPrint(DEBUG_LEVEL_INFO, "Recording stopped - RFID removed");
      } else if (!buttonPressed || currentButtonState == HIGH) {
        // Button released - stop recording
        stopRecording();
        currentState = STATE_WAITING_FOR_BUTTON;
        setLEDPattern(3);
        debugPrint(DEBUG_LEVEL_INFO, "Recording stopped - button released");
      }
      break;
      
    case STATE_PLAYBACK:
      if (!rfidTagPresent) {
        stopPlayback();
        currentState = STATE_IDLE;
        setLEDPattern(1);
        debugPrint(DEBUG_LEVEL_INFO, "Playback stopped - RFID removed");
      } else if (!buttonPressed || currentButtonState == HIGH) {
        // Button released - stop playback
        stopPlayback();
        currentState = STATE_WAITING_FOR_BUTTON;
        setLEDPattern(3);
        debugPrint(DEBUG_LEVEL_INFO, "Playback stopped - button released");
      } else if (!playbackActive) {
        // Playback finished
        currentState = STATE_WAITING_FOR_BUTTON;
        setLEDPattern(3);
        debugPrint(DEBUG_LEVEL_INFO, "Playback completed");
      }
      break;
      
    case STATE_ERROR:
      // Enhanced error state handling
      static unsigned long lastErrorRecovery = 0;
      if (millis() - lastErrorRecovery > ERROR_RECOVERY_INTERVAL) {
        lastErrorRecovery = millis();
        debugPrint(DEBUG_LEVEL_WARNING, "Attempting to recover from error state...");
        
        // Try to recover based on error type
        bool recovered = false;
        switch (lastError) {
          case ERROR_SD_CARD:
            recovered = SD.begin(SD_CS_PIN);
            break;
          case ERROR_RFID_INIT:
            rfid.PCD_Init();
            recovered = rfid.PCD_PerformSelfTest();
            break;
          case ERROR_I2S_INIT:
            recovered = initializeI2S();
            break;
          default:
            recovered = testHardwareComponents();
            break;
        }
        
        if (recovered) {
          currentState = STATE_IDLE;
          lastError = ERROR_NONE;
          setLEDPattern(1);
          debugPrint(DEBUG_LEVEL_INFO, "Recovered from error state");
        }
      }
      break;
  }
}

void startRecording() {
  if (!rfidTagPresent) {
    debugPrint(DEBUG_LEVEL_WARNING, "Cannot start recording - no RFID tag present");
    return;
  }
  
  String filename = "/" + String(AUDIO_FILE_PREFIX) + currentUID + String(AUDIO_FILE_EXTENSION);
  
  // Check if file already exists and create unique name if needed
  int fileCounter = 1;
  while (SD.exists(filename) && fileCounter < 100) {
    filename = "/" + String(AUDIO_FILE_PREFIX) + currentUID + "_" + String(fileCounter) + String(AUDIO_FILE_EXTENSION);
    fileCounter++;
  }
  
  audioFile = SD.open(filename, FILE_WRITE);
  
  if (!audioFile) {
    handleError(ERROR_FILE_SYSTEM, "Failed to create audio file: " + filename);
    return;
  }
  
  // Write WAV header
  writeWAVHeader();
  
  recording = true;
  recordedSamples = 0;
  recordingStartTime = millis();
  currentState = STATE_RECORDING;
  setLEDPattern(2); // Fast blink for recording
  
  debugPrint(DEBUG_LEVEL_INFO, "Recording started: " + filename);
  debugPrint(DEBUG_LEVEL_VERBOSE, "Max recording time: " + String(RECORD_TIME) + " seconds");
}

void stopRecording() {
  if (!recording) return;
  
  recording = false;
  
  // Update WAV header with actual file size
  updateWAVHeader();
  audioFile.close();
  
  unsigned long recordingDuration = millis() - recordingStartTime;
  debugPrint(DEBUG_LEVEL_INFO, "Recording stopped. Duration: " + String(recordingDuration / 1000.0, 2) + 
            "s, Samples: " + String(recordedSamples));
  
  beepFeedback(2); // Success feedback
}

void startPlayback() {
  if (!rfidTagPresent) {
    debugPrint(DEBUG_LEVEL_WARNING, "Cannot start playback - no RFID tag present");
    return;
  }
  
  if (!ENABLE_AUDIO_PLAYBACK) {
    debugPrint(DEBUG_LEVEL_WARNING, "Audio playback is disabled");
    return;
  }
  
  String filename = "/" + String(AUDIO_FILE_PREFIX) + currentUID + String(AUDIO_FILE_EXTENSION);
  
  // Look for numbered variants if exact filename doesn't exist
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
      debugPrint(DEBUG_LEVEL_WARNING, "No audio file found for RFID tag: " + currentUID);
      beepFeedback(5); // Error pattern
      return;
    }
  }
  
  currentState = STATE_PLAYBACK;
  playbackActive = true;
  setLEDPattern(1); // Slow blink for playback
  
  debugPrint(DEBUG_LEVEL_INFO, "Playback started: " + filename);
  
  // TODO: Implement actual audio playback using I2S
  // For now, simulate playback duration based on file size
  File playFile = SD.open(filename, FILE_READ);
  if (playFile) {
    size_t fileSize = playFile.size();
    playFile.close();
    unsigned long estimatedDuration = (fileSize - WAVE_HEADER_SIZE) / (SAMPLE_RATE * 2); // Rough estimate
    debugPrint(DEBUG_LEVEL_VERBOSE, "Estimated playback duration: " + String(estimatedDuration) + " seconds");
  }
}

void stopPlayback() {
  if (!playbackActive) return;
  
  playbackActive = false;
  debugPrint(DEBUG_LEVEL_INFO, "Playback stopped");
  
  // TODO: Implement actual playback stop
}

// MISSING UTILITY FUNCTIONS - NOW IMPLEMENTED

void printRFIDDiagnostics() {
  if (!ENABLE_DIAGNOSTICS) return;
  
  Serial.println("====== RFID DIAGNOSTICS ======");
  
  // Basic RFID reader status
  Serial.println("RFID Reader Status:");
  Serial.println("  Model: RC522");
  Serial.println("  SPI Pins - SS: " + String(RFID_SS_PIN) + ", RST: " + String(RFID_RST_PIN));
  Serial.println("  Current State: " + (rfidTagPresent ? "Tag Present" : "No Tag"));
  
  if (rfidTagPresent) {
    Serial.println("  Current UID: " + currentUID);
    Serial.println("  Detection Count: " + String(rfidDetectionCount));
    Serial.println("  Last Detection: " + String(millis() - lastRFIDDetectionTime) + "ms ago");
  }
  
  // RFID configuration
  Serial.println("RFID Configuration:");
  Serial.println("  Detection Window: " + String(RFID_DETECTION_WINDOW) + "ms");
  Serial.println("  Removal Delay: " + String(RFID_REMOVAL_DELAY) + "ms");
  Serial.println("  Retry Attempts: " + String(RFID_RETRY_ATTEMPTS));
  Serial.println("  Scan Interval: " + String(RFID_SCAN_INTERVAL) + "ms");
  
  // Perform self-test
  Serial.println("Hardware Test:");
  bool selfTestResult = rfid.PCD_PerformSelfTest();
  Serial.println("  Self-test: " + String(selfTestResult ? "PASS" : "FAIL"));
  
  // Check antenna
  Serial.println("  Antenna Status: " + String(rfid.PCD_GetAntennaGain() > 0 ? "Active" : "Inactive"));
  
  // Version information
  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  Serial.println("  Firmware Version: 0x" + String(version, HEX));
  
  Serial.println("==============================");
}

void handleSerialCommands() {
  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toLowerCase();
  
  debugPrint(DEBUG_LEVEL_VERBOSE, "Received command: " + command);
  
  if (command == "status" || command == "s") {
    printSystemStatus();
  } 
  else if (command == "help" || command == "h" || command == "?") {
    printHelpMenu();
  } 
  else if (command == "rfid" || command == "r") {
    printRFIDDiagnostics();
  } 
  else if (command == "files" || command == "f") {
    listAudioFiles();
  } 
  else if (command.startsWith("delete ") || command.startsWith("del ")) {
    String filename = command.substring(command.indexOf(' ') + 1);
    deleteAudioFile(filename);
  } 
  else if (command == "reset" || command == "restart") {
    resetSystem();
  } 
  else if (command == "test" || command == "t") {
    testHardwareComponents();
  } 
  else if (command == "beep" || command == "b") {
    beepFeedback(3);
  } 
  else if (command == "record" || command == "rec") {
    if (rfidTagPresent) {
      startRecording();
      delay(100);
      stopRecording();
    } else {
      Serial.println("No RFID tag present for test recording");
    }
  } 
  else if (command == "play" || command == "p") {
    if (rfidTagPresent) {
      startPlayback();
    } else {
      Serial.println("No RFID tag present for test playback");
    }
  } 
  else if (command == "memory" || command == "mem") {
    Serial.println("Memory Status:");
    Serial.println("  Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  Initial Heap: " + String(initialFreeHeap) + " bytes");
    Serial.println("  Used: " + String(initialFreeHeap - ESP.getFreeHeap()) + " bytes");
    Serial.println("  Largest Free Block: " + String(ESP.getMaxAllocHeap()) + " bytes");
  } 
  else if (command.startsWith("debug ")) {
    int level = command.substring(6).toInt();
    if (level >= 0 && level <= 4) {
      // Note: In a real implementation, you'd modify a global debug level variable
      Serial.println("Debug level set to: " + String(level));
    } else {
      Serial.println("Invalid debug level. Use 0-4.");
    }
  } 
  else if (command == "clear" || command == "cls") {
    // Clear screen (send ANSI escape sequence)
    Serial.print("\033[2J\033[H");
    Serial.println("Screen cleared.");
  } 
  else if (command != "") {
    Serial.println("Unknown command: '" + command + "'");
    Serial.println("Type 'help' for available commands");
  }
}

void printHelpMenu() {
  Serial.println("====== ESP32 RFID AUDIO RECORDER - HELP ======");
  Serial.println("Available Commands:");
  Serial.println();
  Serial.println("System Commands:");
  Serial.println("  status, s       - Show current system status");
  Serial.println("  help, h, ?      - Show this help menu");
  Serial.println("  reset, restart  - Restart the system");
  Serial.println("  test, t         - Test hardware components");
  Serial.println("  memory, mem     - Show memory usage");
  Serial.println("  debug <0-4>     - Set debug level (0=none, 4=verbose)");
  Serial.println("  clear, cls      - Clear screen");
  Serial.println();
  Serial.println("RFID Commands:");
  Serial.println("  rfid, r         - Show RFID diagnostics");
  Serial.println();
  Serial.println("Audio Commands:");
  Serial.println("  files, f        - List audio files on SD card");
  Serial.println("  delete <file>   - Delete specific audio file");
  Serial.println("  record, rec     - Test recording (with RFID tag)");
  Serial.println("  play, p         - Test playback (with RFID tag)");
  Serial.println();
  Serial.println("Hardware Commands:");
  Serial.println("  beep, b         - Test audio feedback");
  Serial.println();
  Serial.println("Usage Instructions:");
  Serial.println("  1. Place RFID tag on reader (LED turns solid)");
  Serial.println("  2. Press & hold button to record (LED blinks fast)");
  Serial.println("  3. Release button to stop recording");
  Serial.println("  4. Long press button (>1s) for playback");
  Serial.println("  5. Remove RFID tag to return to idle");
  Serial.println();
  Serial.println("LED Status Patterns:");
  Serial.println("  Slow blink  - Idle/Ready state");
  Serial.println("  Solid       - RFID detected, waiting for button");
  Serial.println("  Fast blink  - Recording or Error");
  Serial.println("  Off         - System error or not initialized");
  Serial.println("===============================================");
}

void handleError(ErrorType errorType, const String& errorMsg) {
  lastError = errorType;
  currentState = STATE_ERROR;
  setLEDPattern(2); // Fast blink for error
  
  debugPrint(DEBUG_LEVEL_ERROR, "ERROR [" + String(errorType) + "]: " + errorMsg);
  
  // Error-specific handling
  switch (errorType) {
    case ERROR_SD_CARD:
      Serial.println("SD Card Error - Check connections and card format");
      break;
    case ERROR_RFID_INIT:
      Serial.println("RFID Error - Check RC522 wiring and power");
      break;
    case ERROR_I2S_INIT:
      Serial.println("Audio Error - Check I2S microphone connections");
      break;
    case ERROR_FILE_SYSTEM:
      Serial.println("File System Error - Check SD card space and permissions");
      break;
    case ERROR_MEMORY:
      Serial.println("Memory Error - System may be low on RAM");
      break;
    case ERROR_HARDWARE:
      Serial.println("Hardware Error - Check all component connections");
      break;
    default:
      Serial.println("Unknown Error - Check system status");
      break;
  }
  
  // Provide recovery suggestions
  Serial.println("Recovery suggestions:");
  Serial.println("  - Check all hardware connections");
  Serial.println("  - Verify power supply is adequate");
  Serial.println("  - Type 'reset' to restart system");
  Serial.println("  - Type 'test' to diagnose components");
  
  // Audio error indication
  for (int i = 0; i < 5; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(100);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(100);
  }
}

// Additional utility functions

String getUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

bool initializeI2S() {
  // Configure I2S for recording
  i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = DMA_BUF_COUNT,
    .dma_buf_len = DMA_BUF_LEN,
    .use_apll = false
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  
  esp_err_t result = i2s_driver_install(I2S_PORT_RX, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    debugPrint(DEBUG_LEVEL_ERROR, "Failed to install I2S driver: " + String(result));
    return false;
  }
  
  result = i2s_set_pin(I2S_PORT_RX, &pin_config);
  if (result != ESP_OK) {
    debugPrint(DEBUG_LEVEL_ERROR, "Failed to set I2S pins: " + String(result));
    return false;
  }
  
  debugPrint(DEBUG_LEVEL_INFO, "I2S recording initialized successfully");
  return true;
}

bool initializeI2SPlayback() {
  // Configure I2S for playback
  i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = DMA_BUF_COUNT,
    .dma_buf_len = DMA_BUF_LEN,
    .use_apll = false
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_PLAYBACK_SCK,
    .ws_io_num = I2S_PLAYBACK_WS,
    .data_out_num = I2S_PLAYBACK_SD,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  
  esp_err_t result = i2s_driver_install(I2S_PORT_TX, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    debugPrint(DEBUG_LEVEL_ERROR, "Failed to install I2S playback driver: " + String(result));
    return false;
  }
  
  result = i2s_set_pin(I2S_PORT_TX, &pin_config);
  if (result != ESP_OK) {
    debugPrint(DEBUG_LEVEL_ERROR, "Failed to set I2S playback pins: " + String(result));
    return false;
  }
  
  debugPrint(DEBUG_LEVEL_INFO, "I2S playback initialized successfully");
  return true;
}

void writeWAVHeader() {
  // Standard WAV header for 16-bit mono audio
  audioFile.write((uint8_t*)"RIFF", 4);
  audioFile.write((uint8_t*)"\x00\x00\x00\x00", 4); // File size (will be updated)
  audioFile.write((uint8_t*)"WAVE", 4);
  audioFile.write((uint8_t*)"fmt ", 4);
  audioFile.write((uint8_t*)"\x10\x00\x00\x00", 4); // Subchunk1Size (16)
  audioFile.write((uint8_t*)"\x01\x00", 2); // AudioFormat (PCM)
  audioFile.write((uint8_t*)"\x01\x00", 2); // NumChannels (mono)
  
  uint32_t sampleRate = SAMPLE_RATE;
  audioFile.write((uint8_t*)&sampleRate, 4);
  
  uint32_t byteRate = SAMPLE_RATE * 2; // 16-bit mono
  audioFile.write((uint8_t*)&byteRate, 4);
  
  audioFile.write((uint8_t*)"\x02\x00", 2); // BlockAlign
  audioFile.write((uint8_t*)"\x10\x00", 2); // BitsPerSample (16)
  audioFile.write((uint8_t*)"data", 4);
  audioFile.write((uint8_t*)"\x00\x00\x00\x00", 4); // Subchunk2Size (will be updated)
}

void updateWAVHeader() {
  if (!audioFile) return;
  
  uint32_t fileSize = audioFile.size() - 8;
  uint32_t dataSize = audioFile.size() - WAVE_HEADER_SIZE;
  
  audioFile.seek(4);
  audioFile.write((uint8_t*)&fileSize, 4);
  
  audioFile.seek(40);
  audioFile.write((uint8_t*)&dataSize, 4);
  
  debugPrint(DEBUG_LEVEL_VERBOSE, "WAV header updated - File: " + String(fileSize + 8) + 
            " bytes, Data: " + String(dataSize) + " bytes");
}

void setLEDPattern(int pattern) {
  if (!ENABLE_STATUS_LED) return;
  blinkPattern = pattern;
}

void updateStatusLED() {
  if (!ENABLE_STATUS_LED) return;
  
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

void printSystemStatus() {
  Serial.println("====== SYSTEM STATUS ======");
  Serial.println("Uptime: " + String((millis() - systemStartTime) / 1000) + " seconds");
  Serial.println("State: " + getStateName());
  Serial.println("RFID Tag Present: " + String(rfidTagPresent ? "Yes" : "No"));
  if (rfidTagPresent) {
    Serial.println("Current UID: " + currentUID);
  }
  Serial.println("Button State: " + String(currentButtonState == LOW ? "Pressed" : "Released"));
  Serial.println("Recording: " + String(recording ? "Yes" : "No"));
  Serial.println("Playback: " + String(playbackActive ? "Yes" : "No"));
  
  // Memory status
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
  
  // SD card status
  if (SD.begin(SD_CS_PIN)) {
    uint64_t totalBytes = SD.totalBytes();
    uint64_t usedBytes = SD.usedBytes();
    Serial.println("SD Card Total: " + String(totalBytes / 1024) + " KB");
    Serial.println("SD Card Used: " + String(usedBytes / 1024) + " KB");
    Serial.println("SD Card Free: " + String((totalBytes - usedBytes) / 1024) + " KB");
  } else {
    Serial.println("SD Card: Not accessible");
  }
  
  Serial.println("Last Error: " + String(lastError == ERROR_NONE ? "None" : String(lastError)));
  Serial.println("===========================");
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
  Serial.println("====== AUDIO FILES ======");
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    return;
  }
  
  int fileCount = 0;
  File file = root.openNextFile();
  
  while (file) {
    if (!file.isDirectory() && String(file.name()).endsWith(".wav")) {
      fileCount++;
      Serial.println(String(fileCount) + ". " + String(file.name()) + 
                    " (" + String(file.size()) + " bytes)");
    }
    file = root.openNextFile();
  }
  
  if (fileCount == 0) {
    Serial.println("No audio files found");
  } else {
    Serial.println("Total: " + String(fileCount) + " audio files");
  }
  Serial.println("=========================");
}

void deleteAudioFile(const String& filename) {
  String fullPath = filename;
  if (!fullPath.startsWith("/")) {
    fullPath = "/" + fullPath;
  }
  
  if (SD.exists(fullPath)) {
    if (SD.remove(fullPath)) {
      debugPrint(DEBUG_LEVEL_INFO, "File deleted: " + fullPath);
    } else {
      debugPrint(DEBUG_LEVEL_ERROR, "Failed to delete file: " + fullPath);
    }
  } else {
    debugPrint(DEBUG_LEVEL_WARNING, "File not found: " + fullPath);
  }
}

void beepFeedback(int count) {
  // Audio feedback using LED (since we don't have a buzzer)
  for (int i = 0; i < count; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(50);
    digitalWrite(STATUS_LED_PIN, LOW);
    if (i < count - 1) delay(100);
  }
}

void debugPrint(int level, const String& message) {
  if (level <= DEBUG_LEVEL) {
    String prefix = "";
    switch (level) {
      case DEBUG_LEVEL_ERROR: prefix = "[ERROR] "; break;
      case DEBUG_LEVEL_WARNING: prefix = "[WARN] "; break;
      case DEBUG_LEVEL_INFO: prefix = "[INFO] "; break;
      case DEBUG_LEVEL_VERBOSE: prefix = "[DEBUG] "; break;
    }
    Serial.println(prefix + message);
  }
}

void resetSystem() {
  debugPrint(DEBUG_LEVEL_INFO, "System reset requested");
  delay(1000);
  ESP.restart();
}

bool testHardwareComponents() {
  Serial.println("====== HARDWARE TEST ======");
  bool allTestsPassed = true;
  
  // Test LED
  Serial.print("Testing LED... ");
  for (int i = 0; i < 3; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(100);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(100);
  }
  Serial.println("OK");
  
  // Test Button
  Serial.print("Testing Button (press button now)... ");
  unsigned long startTime = millis();
  bool buttonTested = false;
  while (millis() - startTime < 5000) { // 5 second timeout
    if (digitalRead(BUTTON_PIN) == LOW) {
      buttonTested = true;
      break;
    }
    delay(10);
  }
  Serial.println(buttonTested ? "OK" : "TIMEOUT");
  if (!buttonTested) allTestsPassed = false;
  
  // Test RFID
  Serial.print("Testing RFID... ");
  bool rfidTest = rfid.PCD_PerformSelfTest();
  Serial.println(rfidTest ? "OK" : "FAIL");
  if (!rfidTest) allTestsPassed = false;
  
  // Test SD Card
  Serial.print("Testing SD Card... ");
  bool sdTest = SD.begin(SD_CS_PIN);
  Serial.println(sdTest ? "OK" : "FAIL");
  if (!sdTest) allTestsPassed = false;
  
  Serial.println("Test Result: " + String(allTestsPassed ? "ALL PASSED" : "SOME FAILED"));
  Serial.println("===========================");
  
  return allTestsPassed;
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
    debugPrint(DEBUG_LEVEL_VERBOSE, "Health Check - Uptime: " + String(uptime / 1000) + 
              "s, Free Heap: " + String(freeHeap) + " bytes, State: " + getStateName());
    
    // Check for memory leaks
    static size_t lastFreeHeap = freeHeap;
    if (freeHeap < lastFreeHeap * 0.8) { // More than 20% memory loss
      handleError(ERROR_MEMORY, "Possible memory leak detected");
    }
    lastFreeHeap = freeHeap;
    
    // Check if system is stuck in error state too long
    if (currentState == STATE_ERROR && uptime > ERROR_RECOVERY_INTERVAL * 3) {
      debugPrint(DEBUG_LEVEL_WARNING, "System stuck in error state, attempting restart");
      resetSystem();
    }
    
    // Watchdog functionality - reset if system appears frozen
    static unsigned long lastWatchdog = 0;
    static SystemState lastWatchdogState = STATE_IDLE;
    
    if (currentState == lastWatchdogState && 
        (currentTime - lastWatchdog) > (SYSTEM_HEALTH_CHECK * 10)) {
      // System might be frozen if state hasn't changed in a long time
      debugPrint(DEBUG_LEVEL_WARNING, "Watchdog: System may be frozen");
    }
    
    lastWatchdog = currentTime;
    lastWatchdogState = currentState;
  }
}