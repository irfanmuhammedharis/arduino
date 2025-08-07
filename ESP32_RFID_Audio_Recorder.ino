/*
 * Enhanced ESP32 RFID Audio Recorder - Production-Ready Implementation
 * 
 * This is a sophisticated RFID-triggered audio recording system for ESP32 with 
 * professional-grade I2S audio processing and enhanced user experience.
 * 
 * Features:
 * - Dual I2S configuration (I2S0 for recording, I2S1 for playback)
 * - RC522 RFID detection with UID-based file management
 * - Professional audio processing with WAV format
 * - Advanced button handling with debouncing
 * - Comprehensive state machine with error recovery
 * - Dual SPI bus management (VSPI for SD, HSPI for RFID)
 * - Serial command interface for debugging
 * 
 * Hardware Configuration:
 * - SD Card (VSPI): CS=5, MOSI=23, MISO=19, SCK=18
 * - RC522 RFID (HSPI): SS=4, MOSI=13, MISO=12, SCK=16, RST=22
 * - I2S Recording (INMP441): WS=25, SCK=26, RX=27
 * - I2S Playback (Speaker): WS=32, SCK=14, TX=33
 * - Control: BUTTON=21, LED=2
 * 
 * Author: Enhanced ESP32 RFID Audio Recorder
 * Version: 1.0.0
 * Date: 2024
 */

#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>
#include <driver/i2s.h>
#include <math.h>

// ============================================================================
// HARDWARE CONFIGURATION SECTION
// ============================================================================

// SD Card Configuration (VSPI)
#define SD_CS_PIN       5
#define SD_MOSI_PIN     23
#define SD_MISO_PIN     19
#define SD_SCK_PIN      18

// RC522 RFID Configuration (HSPI)
#define RFID_SS_PIN     4
#define RFID_MOSI_PIN   13
#define RFID_MISO_PIN   12
#define RFID_SCK_PIN    16
#define RFID_RST_PIN    22

// I2S Recording Configuration (INMP441)
#define I2S0_WS_PIN     25
#define I2S0_SCK_PIN    26
#define I2S0_RX_DATA_PIN 27

// I2S Playback Configuration (Speaker/Amplifier)
#define I2S1_WS_PIN     32
#define I2S1_SCK_PIN    14
#define I2S1_TX_DATA_PIN 33

// Control GPIO
#define BUTTON_PIN      21
#define LED_PIN         2

// ============================================================================
// AUDIO PARAMETERS SECTION
// ============================================================================

// Audio Specifications
const uint32_t SAMPLE_RATE = 16000;
const uint16_t BITS_PER_SAMPLE = 16;
const uint8_t CHANNELS = 1;
const uint32_t RECORDING_DURATION_MS = 15000;  // 15 seconds
const uint32_t BYTES_PER_SAMPLE = BITS_PER_SAMPLE / 8;
const uint32_t BYTES_PER_SECOND = SAMPLE_RATE * BYTES_PER_SAMPLE * CHANNELS;

// Beep Frequencies and Timing
const uint16_t BEEP_FREQUENCY_1 = 800;   // First countdown beep
const uint16_t BEEP_FREQUENCY_2 = 1000;  // Second countdown beep
const uint16_t BEEP_FREQUENCY_3 = 1200;  // Third countdown beep
const uint16_t SINGLE_BEEP_FREQ = 1000;  // New RFID beep
const uint16_t DELETE_BEEP_FREQ = 600;   // Deletion confirmation
const uint16_t COMPLETION_BEEP_1 = 1500; // Recording completion
const uint16_t COMPLETION_BEEP_2 = 1600; // Recording completion
const uint16_t COMPLETION_BEEP_3 = 1700; // Recording completion

const uint16_t BEEP_DURATION = 500;      // Standard beep length (ms)
const uint16_t BEEP_PAUSE = 300;         // Pause between beeps (ms)
const float BEEP_AMPLITUDE = 0.3f;       // Beep volume level (0.0-1.0)

// ============================================================================
// TIMING CONFIGURATION SECTION
// ============================================================================

const uint32_t RFID_CHECK_INTERVAL = 100;      // RFID polling rate (ms)
const uint32_t BUTTON_DEBOUNCE_MS = 50;        // Button debounce time (ms)
const uint32_t RFID_GRACE_PERIOD = 3000;       // Tag presence grace period (ms)
const uint32_t RFID_REMOVAL_THRESHOLD = 4000;  // Tag removal detection (ms)
const uint32_t SPI_SWITCH_DELAY_US = 100;      // SPI bus switching delay (us)
const uint32_t I2S_BUFFER_SIZE = 1024;         // I2S DMA buffer size

// ============================================================================
// GLOBAL VARIABLES SECTION
// ============================================================================

// System State Enumeration
enum SystemState {
  STATE_IDLE,
  STATE_RECORDING,
  STATE_PLAYING,
  STATE_NEW_BEEP,
  STATE_DELETE_BEEP,
  STATE_RECORDING_BEEPS,
  STATE_ERROR
};

// Global State Variables
SystemState currentState = STATE_IDLE;
uint32_t stateStartTime = 0;
uint32_t lastRfidCheck = 0;
uint32_t lastButtonCheck = 0;
uint32_t beepStartTime = 0;
uint32_t recordingStartTime = 0;

// RFID Management
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
String currentRfidUid = "";
String lastDetectedUid = "";
uint32_t lastRfidDetectionTime = 0;
bool rfidPresent = false;
bool rfidJustDetected = false;

// Button State Management
bool buttonPressed = false;
bool lastButtonState = false;
uint32_t lastButtonPressTime = 0;
bool buttonJustPressed = false;

// Audio Buffers and Management
static int16_t audioBuffer[I2S_BUFFER_SIZE];
static int16_t playbackBuffer[I2S_BUFFER_SIZE];
File audioFile;
bool isRecording = false;
bool isPlaying = false;
uint32_t recordedSamples = 0;
uint32_t playbackPosition = 0;

// SPI Bus Management
SPIClass vspi(VSPI);
SPIClass hspi(HSPI);
bool sdCardReady = false;
bool rfidReady = false;

// Beep Generation Variables
uint8_t currentBeepCount = 0;
uint8_t totalBeeps = 0;
uint16_t currentBeepFreq = 0;
bool beepActive = false;
uint32_t beepPhase = 0;

// Memory and Error Management
uint32_t freeHeapStart = 0;
uint8_t errorRecoveryAttempts = 0;
const uint8_t MAX_RECOVERY_ATTEMPTS = 3;

// ============================================================================
// FUNCTION DECLARATIONS SECTION
// ============================================================================

// System Initialization
void setupHardware();
void setupSPI();
void setupI2S();
void setupSDCard();
void setupRFID();
void setupGPIO();

// Main Loop Functions
void updateSystemState();
void handleRFIDDetection();
void handleButtonInput();
void processAudioOperations();

// State Machine Functions
void handleIdleState();
void handleRecordingState();
void handlePlayingState();
void handleNewBeepState();
void handleDeleteBeepState();
void handleRecordingBeepsState();
void handleErrorState();

// Audio Processing Functions
void startRecording();
void stopRecording();
void startPlayback(const String& filename);
void stopPlayback();
void generateBeep(uint16_t frequency, uint32_t duration);
void processI2SRecording();
void processI2SPlayback();

// RFID Management Functions
String readRFIDUid();
bool checkRFIDPresence();
void updateRFIDState();
String formatRFIDFilename(const String& uid);

// Button Handling Functions
void updateButtonState();
bool isButtonJustPressed();
bool isButtonCurrentlyPressed();

// File Management Functions
bool createWAVFile(const String& filename);
bool writeWAVHeader(File& file, uint32_t dataSize);
bool fileExists(const String& filename);
bool deleteFile(const String& filename);

// SPI Bus Management Functions
void selectSDCardSPI();
void selectRFIDSPI();
void delaySPISwitch();

// Beep Generation Functions
void playBeepSequence(uint16_t* frequencies, uint8_t count);
void playCompletionBeeps();
void updateBeepGeneration();
int16_t generateSineWave(uint16_t frequency, uint32_t sampleIndex);

// Utility Functions
void printSystemStatus();
void printMemoryStatus();
void handleSerialCommands();
void blinkLED(uint32_t duration);
void enterErrorState(const String& errorMessage);
void resetSystem();

// ============================================================================
// SETUP FUNCTION
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 RFID Audio Recorder ===");
  Serial.println("Initializing system...");
  
  // Record initial heap size
  freeHeapStart = ESP.getFreeHeap();
  
  // Initialize hardware components
  setupHardware();
  
  // Display system status
  printSystemStatus();
  
  Serial.println("System ready! Waiting for RFID tags...");
  Serial.println("Commands: 1=record test, 2=list files, 3=status, test=button test, beep=audio test");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Handle serial commands
  handleSerialCommands();
  
  // Update system timers
  uint32_t currentTime = millis();
  
  // Check for RFID detection
  if (currentTime - lastRfidCheck >= RFID_CHECK_INTERVAL) {
    handleRFIDDetection();
    lastRfidCheck = currentTime;
  }
  
  // Check button state
  if (currentTime - lastButtonCheck >= 10) {  // 10ms polling for responsive button
    handleButtonInput();
    lastButtonCheck = currentTime;
  }
  
  // Process audio operations
  processAudioOperations();
  
  // Update system state
  updateSystemState();
  
  // Update beep generation
  updateBeepGeneration();
  
  // Brief yield to prevent watchdog timeout
  yield();
}

// ============================================================================
// HARDWARE INITIALIZATION FUNCTIONS
// ============================================================================

void setupHardware() {
  setupGPIO();
  setupSPI();
  setupI2S();
  setupSDCard();
  setupRFID();
}

void setupGPIO() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("✓ GPIO initialized");
}

void setupSPI() {
  // Initialize VSPI for SD Card
  vspi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  
  // Initialize HSPI for RFID
  hspi.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  pinMode(RFID_SS_PIN, OUTPUT);
  digitalWrite(RFID_SS_PIN, HIGH);
  
  Serial.println("✓ SPI buses initialized");
}

void setupI2S() {
  // Configure I2S0 for recording (INMP441)
  i2s_config_t i2s0_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = I2S_BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t i2s0_pins = {
    .bck_io_num = I2S0_SCK_PIN,
    .ws_io_num = I2S0_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S0_RX_DATA_PIN
  };
  
  // Configure I2S1 for playback
  i2s_config_t i2s1_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = I2S_BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t i2s1_pins = {
    .bck_io_num = I2S1_SCK_PIN,
    .ws_io_num = I2S1_WS_PIN,
    .data_out_num = I2S1_TX_DATA_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  
  // Install and start I2S drivers
  esp_err_t err0 = i2s_driver_install(I2S_NUM_0, &i2s0_config, 0, NULL);
  esp_err_t err1 = i2s_driver_install(I2S_NUM_1, &i2s1_config, 0, NULL);
  
  if (err0 != ESP_OK || err1 != ESP_OK) {
    enterErrorState("I2S driver installation failed");
    return;
  }
  
  i2s_set_pin(I2S_NUM_0, &i2s0_pins);
  i2s_set_pin(I2S_NUM_1, &i2s1_pins);
  
  Serial.println("✓ I2S audio system initialized");
}

void setupSDCard() {
  selectSDCardSPI();
  
  if (!SD.begin(SD_CS_PIN, vspi, 4000000)) {
    Serial.println("✗ SD Card initialization failed");
    sdCardReady = false;
    return;
  }
  
  sdCardReady = true;
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("✓ SD Card initialized - Size: %lluMB\n", cardSize);
}

void setupRFID() {
  selectRFIDSPI();
  
  rfid.PCD_Init();
  
  // Test RFID communication
  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("✗ RFID initialization failed");
    rfidReady = false;
    return;
  }
  
  rfidReady = true;
  Serial.printf("✓ RFID initialized - Version: 0x%02X\n", version);
}

// ============================================================================
// STATE MACHINE FUNCTIONS
// ============================================================================

void updateSystemState() {
  switch (currentState) {
    case STATE_IDLE:
      handleIdleState();
      break;
    case STATE_RECORDING:
      handleRecordingState();
      break;
    case STATE_PLAYING:
      handlePlayingState();
      break;
    case STATE_NEW_BEEP:
      handleNewBeepState();
      break;
    case STATE_DELETE_BEEP:
      handleDeleteBeepState();
      break;
    case STATE_RECORDING_BEEPS:
      handleRecordingBeepsState();
      break;
    case STATE_ERROR:
      handleErrorState();
      break;
  }
}

void handleIdleState() {
  // Check for RFID detection
  if (rfidJustDetected && !currentRfidUid.isEmpty()) {
    String filename = formatRFIDFilename(currentRfidUid);
    
    if (buttonJustPressed) {
      // Delete and re-record mode
      if (fileExists(filename)) {
        deleteFile(filename);
        currentState = STATE_DELETE_BEEP;
        stateStartTime = millis();
        generateBeep(DELETE_BEEP_FREQ, BEEP_DURATION);
      } else {
        // No file to delete, go straight to recording beeps
        currentState = STATE_RECORDING_BEEPS;
        stateStartTime = millis();
        currentBeepCount = 0;
        totalBeeps = 3;
      }
    } else {
      // Check if file exists
      if (fileExists(filename)) {
        // Play existing recording
        startPlayback(filename);
        currentState = STATE_PLAYING;
        stateStartTime = millis();
      } else {
        // New RFID, play beep and start recording countdown
        currentState = STATE_NEW_BEEP;
        stateStartTime = millis();
        generateBeep(SINGLE_BEEP_FREQ, BEEP_DURATION);
      }
    }
    
    rfidJustDetected = false;
    buttonJustPressed = false;
  }
}

void handleRecordingState() {
  uint32_t elapsed = millis() - recordingStartTime;
  
  // Update LED (blink during recording)
  digitalWrite(LED_PIN, (elapsed / 500) % 2);
  
  // Check if recording duration exceeded
  if (elapsed >= RECORDING_DURATION_MS) {
    stopRecording();
    currentState = STATE_IDLE;
    playCompletionBeeps();
    digitalWrite(LED_PIN, LOW);
  }
  
  // Process I2S recording
  processI2SRecording();
}

void handlePlayingState() {
  // Stop if RFID removed
  if (!rfidPresent) {
    stopPlayback();
    currentState = STATE_IDLE;
    return;
  }
  
  // Continue playback processing
  processI2SPlayback();
  
  // Check if playback finished
  if (!isPlaying) {
    currentState = STATE_IDLE;
  }
}

void handleNewBeepState() {
  uint32_t elapsed = millis() - stateStartTime;
  
  if (elapsed >= BEEP_DURATION + BEEP_PAUSE) {
    // Start recording countdown
    currentState = STATE_RECORDING_BEEPS;
    stateStartTime = millis();
    currentBeepCount = 0;
    totalBeeps = 3;
  }
}

void handleDeleteBeepState() {
  uint32_t elapsed = millis() - stateStartTime;
  
  if (elapsed >= BEEP_DURATION + BEEP_PAUSE) {
    // Start recording countdown after delete confirmation
    currentState = STATE_RECORDING_BEEPS;
    stateStartTime = millis();
    currentBeepCount = 0;
    totalBeeps = 3;
  }
}

void handleRecordingBeepsState() {
  uint32_t elapsed = millis() - stateStartTime;
  uint32_t beepCycleTime = BEEP_DURATION + BEEP_PAUSE;
  
  if (currentBeepCount < totalBeeps) {
    uint32_t currentCycle = elapsed / beepCycleTime;
    
    if (currentCycle > currentBeepCount) {
      // Play next beep
      uint16_t frequencies[] = {BEEP_FREQUENCY_1, BEEP_FREQUENCY_2, BEEP_FREQUENCY_3};
      generateBeep(frequencies[currentBeepCount], BEEP_DURATION);
      currentBeepCount++;
    }
  } else {
    // All beeps played, start recording
    String filename = formatRFIDFilename(currentRfidUid);
    startRecording();
    if (createWAVFile(filename)) {
      currentState = STATE_RECORDING;
      recordingStartTime = millis();
      digitalWrite(LED_PIN, HIGH);
    } else {
      enterErrorState("Failed to create recording file");
    }
  }
}

void handleErrorState() {
  // Blink LED rapidly in error state
  digitalWrite(LED_PIN, (millis() / 200) % 2);
  
  // Try to recover after timeout
  if (millis() - stateStartTime > 5000) {
    errorRecoveryAttempts++;
    if (errorRecoveryAttempts < MAX_RECOVERY_ATTEMPTS) {
      Serial.println("Attempting system recovery...");
      currentState = STATE_IDLE;
      digitalWrite(LED_PIN, LOW);
    } else {
      Serial.println("Maximum recovery attempts reached. Manual intervention required.");
      // Stay in error state
    }
  }
}

// ============================================================================
// RFID MANAGEMENT FUNCTIONS
// ============================================================================

void handleRFIDDetection() {
  if (!rfidReady) return;
  
  selectRFIDSPI();
  updateRFIDState();
}

void updateRFIDState() {
  String detectedUid = readRFIDUid();
  uint32_t currentTime = millis();
  
  if (!detectedUid.isEmpty()) {
    // RFID detected
    lastRfidDetectionTime = currentTime;
    
    if (detectedUid != lastDetectedUid) {
      // New RFID detected
      currentRfidUid = detectedUid;
      lastDetectedUid = detectedUid;
      rfidJustDetected = true;
      rfidPresent = true;
      Serial.println("RFID detected: " + detectedUid);
    } else {
      // Same RFID still present
      rfidPresent = true;
    }
  } else {
    // No RFID detected
    if (rfidPresent && (currentTime - lastRfidDetectionTime > RFID_REMOVAL_THRESHOLD)) {
      // RFID removed
      rfidPresent = false;
      currentRfidUid = "";
      lastDetectedUid = "";
      Serial.println("RFID removed");
    }
  }
}

String readRFIDUid() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return "";
  }
  
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  return uid;
}

String formatRFIDFilename(const String& uid) {
  return "/RFID_" + uid + ".wav";
}

// ============================================================================
// BUTTON HANDLING FUNCTIONS
// ============================================================================

void handleButtonInput() {
  updateButtonState();
}

void updateButtonState() {
  bool currentButtonReading = !digitalRead(BUTTON_PIN);  // Inverted due to pull-up
  uint32_t currentTime = millis();
  
  // Debouncing logic
  if (currentButtonReading != lastButtonState) {
    lastButtonPressTime = currentTime;
  }
  
  if ((currentTime - lastButtonPressTime) > BUTTON_DEBOUNCE_MS) {
    if (currentButtonReading != buttonPressed) {
      buttonPressed = currentButtonReading;
      
      if (buttonPressed) {
        buttonJustPressed = true;
        Serial.println("Button pressed");
      }
    }
  }
  
  lastButtonState = currentButtonReading;
}

bool isButtonJustPressed() {
  if (buttonJustPressed) {
    buttonJustPressed = false;
    return true;
  }
  return false;
}

bool isButtonCurrentlyPressed() {
  return buttonPressed;
}

// ============================================================================
// AUDIO PROCESSING FUNCTIONS
// ============================================================================

void processAudioOperations() {
  // This function is called frequently to handle real-time audio processing
  // The actual I2S processing is handled in the respective state functions
}

void startRecording() {
  if (!sdCardReady) {
    enterErrorState("SD Card not ready for recording");
    return;
  }
  
  isRecording = true;
  recordedSamples = 0;
  
  // Clear I2S buffers
  i2s_zero_dma_buffer(I2S_NUM_0);
  
  Serial.println("Recording started...");
}

void stopRecording() {
  isRecording = false;
  
  // Close audio file and update WAV header
  if (audioFile) {
    uint32_t dataSize = recordedSamples * BYTES_PER_SAMPLE;
    audioFile.seek(0);
    writeWAVHeader(audioFile, dataSize);
    audioFile.close();
    
    Serial.printf("Recording stopped. Saved %d samples (%d bytes)\n", 
                  recordedSamples, dataSize);
  }
}

void startPlayback(const String& filename) {
  if (!sdCardReady) {
    enterErrorState("SD Card not ready for playback");
    return;
  }
  
  selectSDCardSPI();
  audioFile = SD.open(filename, FILE_READ);
  
  if (!audioFile) {
    enterErrorState("Failed to open audio file: " + filename);
    return;
  }
  
  // Skip WAV header (44 bytes)
  audioFile.seek(44);
  
  isPlaying = true;
  playbackPosition = 0;
  
  // Clear I2S playback buffer
  i2s_zero_dma_buffer(I2S_NUM_1);
  
  Serial.println("Playback started: " + filename);
}

void stopPlayback() {
  isPlaying = false;
  
  if (audioFile) {
    audioFile.close();
  }
  
  // Clear I2S playback buffer
  i2s_zero_dma_buffer(I2S_NUM_1);
  
  Serial.println("Playback stopped");
}

void processI2SRecording() {
  if (!isRecording) return;
  
  size_t bytesRead = 0;
  esp_err_t result = i2s_read(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytesRead, 0);
  
  if (result == ESP_OK && bytesRead > 0) {
    size_t samplesRead = bytesRead / BYTES_PER_SAMPLE;
    
    // Write to SD card
    selectSDCardSPI();
    if (audioFile) {
      size_t bytesWritten = audioFile.write((uint8_t*)audioBuffer, bytesRead);
      if (bytesWritten == bytesRead) {
        recordedSamples += samplesRead;
      } else {
        enterErrorState("SD card write error during recording");
      }
    }
  }
}

void processI2SPlayback() {
  if (!isPlaying || !audioFile) return;
  
  size_t bytesToRead = sizeof(playbackBuffer);
  size_t bytesRead = audioFile.read((uint8_t*)playbackBuffer, bytesToRead);
  
  if (bytesRead > 0) {
    size_t bytesWritten = 0;
    esp_err_t result = i2s_write(I2S_NUM_1, playbackBuffer, bytesRead, &bytesWritten, 0);
    
    if (result != ESP_OK || bytesWritten != bytesRead) {
      enterErrorState("I2S playback error");
    }
    
    playbackPosition += bytesRead;
  } else {
    // End of file reached
    stopPlayback();
  }
}

// ============================================================================
// BEEP GENERATION FUNCTIONS
// ============================================================================

void generateBeep(uint16_t frequency, uint32_t duration) {
  currentBeepFreq = frequency;
  beepStartTime = millis();
  beepActive = true;
  beepPhase = 0;
  
  Serial.printf("Playing beep: %dHz for %dms\n", frequency, duration);
}

void updateBeepGeneration() {
  if (!beepActive) return;
  
  uint32_t elapsed = millis() - beepStartTime;
  if (elapsed >= BEEP_DURATION) {
    beepActive = false;
    // Clear I2S buffer to stop beep
    i2s_zero_dma_buffer(I2S_NUM_1);
    return;
  }
  
  // Generate beep samples
  for (int i = 0; i < I2S_BUFFER_SIZE; i++) {
    playbackBuffer[i] = generateSineWave(currentBeepFreq, beepPhase);
    beepPhase++;
  }
  
  // Send to I2S
  size_t bytesWritten = 0;
  i2s_write(I2S_NUM_1, playbackBuffer, sizeof(playbackBuffer), &bytesWritten, 0);
}

int16_t generateSineWave(uint16_t frequency, uint32_t sampleIndex) {
  float angle = 2.0 * M_PI * frequency * sampleIndex / SAMPLE_RATE;
  float amplitude = BEEP_AMPLITUDE * 32767.0;  // 16-bit amplitude
  return (int16_t)(amplitude * sin(angle));
}

void playCompletionBeeps() {
  // This will be handled by the beep generation system
  // For now, just play a single completion beep
  generateBeep(COMPLETION_BEEP_1, BEEP_DURATION);
}

// ============================================================================
// FILE MANAGEMENT FUNCTIONS
// ============================================================================

bool createWAVFile(const String& filename) {
  selectSDCardSPI();
  
  audioFile = SD.open(filename, FILE_WRITE);
  if (!audioFile) {
    return false;
  }
  
  // Write initial WAV header (will be updated when recording stops)
  writeWAVHeader(audioFile, 0);
  
  return true;
}

bool writeWAVHeader(File& file, uint32_t dataSize) {
  // WAV header structure
  uint32_t fileSize = dataSize + 36;
  uint32_t byteRate = SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8);
  uint16_t blockAlign = CHANNELS * (BITS_PER_SAMPLE / 8);
  
  file.write("RIFF", 4);
  file.write((uint8_t*)&fileSize, 4);
  file.write("WAVE", 4);
  file.write("fmt ", 4);
  
  uint32_t fmtSize = 16;
  file.write((uint8_t*)&fmtSize, 4);
  
  uint16_t audioFormat = 1;  // PCM
  file.write((uint8_t*)&audioFormat, 2);
  file.write((uint8_t*)&CHANNELS, 2);
  file.write((uint8_t*)&SAMPLE_RATE, 4);
  file.write((uint8_t*)&byteRate, 4);
  file.write((uint8_t*)&blockAlign, 2);
  file.write((uint8_t*)&BITS_PER_SAMPLE, 2);
  
  file.write("data", 4);
  file.write((uint8_t*)&dataSize, 4);
  
  return true;
}

bool fileExists(const String& filename) {
  selectSDCardSPI();
  return SD.exists(filename);
}

bool deleteFile(const String& filename) {
  selectSDCardSPI();
  bool result = SD.remove(filename);
  if (result) {
    Serial.println("Deleted file: " + filename);
  } else {
    Serial.println("Failed to delete file: " + filename);
  }
  return result;
}

// ============================================================================
// SPI BUS MANAGEMENT FUNCTIONS
// ============================================================================

void selectSDCardSPI() {
  digitalWrite(RFID_SS_PIN, HIGH);  // Deselect RFID
  delaySPISwitch();
  digitalWrite(SD_CS_PIN, LOW);     // Select SD Card
}

void selectRFIDSPI() {
  digitalWrite(SD_CS_PIN, HIGH);    // Deselect SD Card
  delaySPISwitch();
  digitalWrite(RFID_SS_PIN, LOW);   // Select RFID
}

void delaySPISwitch() {
  delayMicroseconds(SPI_SWITCH_DELAY_US);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void printSystemStatus() {
  Serial.println("\n=== SYSTEM STATUS ===");
  Serial.printf("Free Heap: %d bytes (Start: %d)\n", ESP.getFreeHeap(), freeHeapStart);
  Serial.printf("SD Card: %s\n", sdCardReady ? "Ready" : "Failed");
  Serial.printf("RFID: %s\n", rfidReady ? "Ready" : "Failed");
  Serial.printf("Current State: %d\n", currentState);
  Serial.printf("RFID Present: %s\n", rfidPresent ? "Yes" : "No");
  Serial.printf("Button Pressed: %s\n", buttonPressed ? "Yes" : "No");
  Serial.println("====================\n");
}

void printMemoryStatus() {
  uint32_t currentHeap = ESP.getFreeHeap();
  uint32_t usedMemory = freeHeapStart - currentHeap;
  Serial.printf("Memory - Free: %d, Used: %d\n", currentHeap, usedMemory);
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "1") {
      Serial.println("Manual recording test...");
      startRecording();
      if (createWAVFile("/test_recording.wav")) {
        currentState = STATE_RECORDING;
        recordingStartTime = millis();
      }
    } else if (command == "2") {
      Serial.println("SD Card files:");
      selectSDCardSPI();
      File root = SD.open("/");
      File file = root.openNextFile();
      while (file) {
        Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
        file = root.openNextFile();
      }
      root.close();
    } else if (command == "3") {
      printSystemStatus();
      printMemoryStatus();
    } else if (command == "test") {
      Serial.println("Button test - Press button now...");
      for (int i = 0; i < 50; i++) {
        updateButtonState();
        if (isButtonJustPressed()) {
          Serial.println("Button test PASSED!");
          break;
        }
        delay(100);
      }
    } else if (command == "beep") {
      Serial.println("Audio test - Playing test beep...");
      generateBeep(1000, 1000);
    } else if (command.startsWith("v")) {
      // Volume control (future enhancement)
      Serial.println("Volume control not yet implemented");
    }
  }
}

void blinkLED(uint32_t duration) {
  digitalWrite(LED_PIN, HIGH);
  delay(duration);
  digitalWrite(LED_PIN, LOW);
}

void enterErrorState(const String& errorMessage) {
  Serial.println("ERROR: " + errorMessage);
  currentState = STATE_ERROR;
  stateStartTime = millis();
  
  // Stop any ongoing operations
  if (isRecording) {
    stopRecording();
  }
  if (isPlaying) {
    stopPlayback();
  }
}

void resetSystem() {
  Serial.println("Resetting system...");
  
  // Reset state variables
  currentState = STATE_IDLE;
  isRecording = false;
  isPlaying = false;
  rfidPresent = false;
  currentRfidUid = "";
  buttonPressed = false;
  errorRecoveryAttempts = 0;
  
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("System reset complete");
}