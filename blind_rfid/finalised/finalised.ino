/*
 * ESP32 RFID-Triggered Audio Recorder — PRODUCTION BUILD
 * Behavior:
 *  • Registered tag → Auto-PLAY full 15s audio (do NOT stop on tag removal) 🎧
 *  • Unregistered tag → Auto-RECORD 15s 🎙️
 *  • While playing → lock out ALL other actions (wipe/record/serial/RFID) 🔒
 *  • Hold button + tag present → DELETE ONLY (when NOT playing) 🧹
 *  • One playback per “presence”: no auto-replay loops while tag stays in field
 *
 * Fixes:
 *  • Playback state-order bug fixed (set PLAYING before call; loop runs to EOF)
 *  • Hard lock during playback; commands/wipe/RFID ignored
 *  • Per-presence gating to avoid endless replays
 *  • CS lines forced HIGH on SPI role switches
 *  • Consistent state cleanup on error/early returns
 *
 * Version: 1.5.0-prod
 * Date: 2025-08-08
 * User: irfanmuhammedharis
 *
 * ⚠️ Pins: NO CHANGES in this build (same as your previous sketch)
 *    If you see boot issues, consider moving RC522 MISO from GPIO12 to GPIO17.
 */

#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <driver/i2s.h>
#include <MFRC522.h>

// ══════════════════════════════════════════════════════════════
// 🎯 HARDWARE CONFIGURATION
// ══════════════════════════════════════════════════════════════

// SD Card (VSPI)
const uint8_t SD_CS_PIN    = 5;
const uint8_t SD_MOSI_PIN  = 23;
const uint8_t SD_MISO_PIN  = 19;
const uint8_t SD_SCK_PIN   = 18;
const uint32_t SD_SPI_FREQ = 8000000;

// RC522 RFID (HSPI on single SPI instance switching)
const uint8_t RFID_RST_PIN  = 22;
const uint8_t RFID_SS_PIN   = 4;
const uint8_t RFID_MOSI_PIN = 13;
const uint8_t RFID_MISO_PIN = 12;  // ⚠️ strap pin on some boards; see note above
const uint8_t RFID_SCK_PIN  = 16;
const uint32_t RFID_SPI_FREQ = 1000000;

// Control Button
const uint8_t BUTTON_PIN = 21;

// I2S Audio
const uint8_t I2S0_WS_PIN      = 25;
const uint8_t I2S0_SCK_PIN     = 26;
const uint8_t I2S0_RX_DATA_PIN = 27;

const uint8_t I2S1_WS_PIN      = 32;
const uint8_t I2S1_SCK_PIN     = 14;
const uint8_t I2S1_TX_DATA_PIN = 33;

// ══════════════════════════════════════════════════════════════
// 🎵 AUDIO PARAMETERS
// ══════════════════════════════════════════════════════════════
const uint32_t SAMPLE_RATE     = 16000;
const uint8_t  BITS_PER_SAMPLE = 16;   // adjust if your mic needs 32 → 16 conversion
const uint8_t  CHANNELS        = 1;
const uint8_t  RECORD_SECONDS  = 15;
const uint16_t I2S_BUFFER_SIZE = 1024;
const uint8_t  I2S_DMA_BUFFERS = 8;

// ══════════════════════════════════════════════════════════════
// ⏱️ TIMING
// ══════════════════════════════════════════════════════════════
const uint32_t RFID_CHECK_INTERVAL   = 100;   // 100ms
const uint32_t BUTTON_DEBOUNCE_MS    = 50;
const uint32_t RFID_DEBOUNCE_MS      = 5000;  // grace to consider removal
const uint32_t RFID_DETECTION_WINDOW = 10000; // overall window
const uint32_t SERIAL_CHECK_INTERVAL = 150;
const uint32_t SPI_SWITCH_DELAY_US   = 20;
const uint32_t WATCHDOG_FEED_INTERVAL = 1000;

// ══════════════════════════════════════════════════════════════
// 💾 WAV HEADER
// ══════════════════════════════════════════════════════════════
struct __attribute__((packed)) WAVHeader {
  char     chunkID[4];       // "RIFF"
  uint32_t chunkSize;        // file size - 8
  char     format[4];        // "WAVE"
  char     subchunk1ID[4];   // "fmt "
  uint32_t subchunk1Size;    // 16
  uint16_t audioFormat;      // 1
  uint16_t numChannels;      // 1
  uint32_t sampleRate;       // 16000
  uint32_t byteRate;         // sr * ch * bps/8
  uint16_t blockAlign;       // ch * bps/8
  uint16_t bitsPerSample;    // 16
  char     subchunk2ID[4];   // "data"
  uint32_t subchunk2Size;    // data bytes
};

// ══════════════════════════════════════════════════════════════
// 🔄 STATES / SPI BUS
// ══════════════════════════════════════════════════════════════
enum SystemState { STATE_IDLE, STATE_RECORDING, STATE_PLAYING, STATE_ERROR };
enum SPIBusState { SPI_BUS_SD, SPI_BUS_RFID, SPI_BUS_NONE };

// ══════════════════════════════════════════════════════════════
// 🌐 GLOBALS
// ══════════════════════════════════════════════════════════════
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);
File audioFile;

SystemState currentState = STATE_IDLE;
SPIBusState currentSPIBus = SPI_BUS_NONE;
float volume = 0.7f;
bool fileSystemBusy = false;

unsigned long lastRFIDCheck = 0;
unsigned long lastButtonCheck = 0;
unsigned long lastSerialCheck = 0;
unsigned long lastRFIDDetected = 0;
unsigned long rfidFirstDetected = 0;
unsigned long recordingStartTime = 0;
unsigned long lastWatchdogFeed = 0;

bool lastButtonState = HIGH;
unsigned long lastButtonChange = 0;

String currentRFIDUID = "";
bool rfidTagPresent = false;
bool rfidStillDetecting = false;

char currentWavFilename[64] = "";
uint32_t freeHeapMin = UINT32_MAX;

// Per-presence gating to avoid endless replay
String presenceUID = "";
bool   playedThisPresence = false;

// ══════════════════════════════════════════════════════════════
// 🔧 DECLARATIONS
// ══════════════════════════════════════════════════════════════
bool initSDCard();
bool initRFID();
bool initI2SRecording();
bool initI2SPlayback();
void cleanupI2SPlayback();

void switchToSDSPI();
void switchToRFIDSPI();
void switchSPIBus(SPIBusState newBus);
inline void csAllHigh();

void recordAudioForRFID(const String& rfidUID);
void playbackAudioForRFID(const String& rfidUID);
void checkRFIDTag();
void checkButton();
void handleSystemStates();
void processSerialCommands();
void feedWatchdog();

String rfidUIDToString(MFRC522::Uid *uid);
String getFileNameForRFID(const String& rfidUID);
bool audioFileExistsForRFID(const String& rfidUID);
void setVolume(int volPercent);
void recordManualAudio(const String& filename);
void listSDFiles();
void printSystemStatus();

void writeWAVHeader(File &file);
void updateWAVHeader(File &file, uint32_t dataSize);
bool validateWAVFile(const String& filename);
void printWAVFileInfo(const String& filename);
void validateAllWAVFiles();

bool isMemoryLow();
bool waitForFileSystemReady(uint32_t timeoutMs);

void wipeRFIDDataForUID(const String& rfidUID);

// ══════════════════════════════════════════════════════════════
// 🚀 SETUP
// ══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("\n╔════════════════════════════════════════════════════════════╗"));
  Serial.println(F("║  ESP32 RFID Audio v1.5.0 (Production)                      ║"));
  Serial.println(F("╚════════════════════════════════════════════════════════════╝\n"));

  Serial.print(F("[🔍] WAV Header size: ")); Serial.println(sizeof(WAVHeader));
  freeHeapMin = ESP.getFreeHeap();
  Serial.print(F("[💾] Initial free heap: ")); Serial.print(ESP.getFreeHeap()); Serial.println(F(" bytes"));

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SD_CS_PIN, OUTPUT);    digitalWrite(SD_CS_PIN, HIGH);
  pinMode(RFID_SS_PIN, OUTPUT);  digitalWrite(RFID_SS_PIN, HIGH);
  Serial.println(F("[✓] Button + CS lines configured"));

  if (!initSDCard()) {
    Serial.println(F("[💥] FATAL: SD Card init failed"));
    while (true) { delay(1000); Serial.print(F(".")); }
  }
  if (!initRFID()) {
    Serial.println(F("[💥] FATAL: RFID init failed"));
    while (true) { delay(1000); Serial.print(F(".")); }
  }
  if (!initI2SRecording()) {
    Serial.println(F("[💥] FATAL: I2S Recording init failed"));
    while (true) { delay(1000); Serial.print(F(".")); }
  }

  Serial.println(F("\n▶️ Behavior:"));
  Serial.println(F("   • Known tag → Auto-PLAY to end (ignore removal); lock all else"));
  Serial.println(F("   • New tag   → Auto-RECORD 15s (when not holding button)"));
  Serial.println(F("   • Hold button + tag → DELETE ONLY (when not playing)"));
  Serial.println(F("   • One playback per presence — replay only after removal\n"));

  Serial.println(F("[📋] Commands: 1=record, 2=list, 3=status, 4=validate, info <f>, v<0-100>\n"));
}

// ══════════════════════════════════════════════════════════════
void loop() {
  if (millis() - lastWatchdogFeed >= WATCHDOG_FEED_INTERVAL) {
    feedWatchdog();
    lastWatchdogFeed = millis();
  }

  uint32_t currentHeap = ESP.getFreeHeap();
  if (currentHeap < freeHeapMin) freeHeapMin = currentHeap;

  if (millis() - lastButtonCheck >= 10) {
    checkButton();
    lastButtonCheck = millis();
  }

  if (millis() - lastRFIDCheck >= RFID_CHECK_INTERVAL) {
    checkRFIDTag();
    lastRFIDCheck = millis();
  }

  handleSystemStates();

  if (millis() - lastSerialCheck >= SERIAL_CHECK_INTERVAL) {
    processSerialCommands();
    lastSerialCheck = millis();
  }

  yield();
}

// ══════════════════════════════════════════════════════════════
// 🤖 STATE MACHINE
// ══════════════════════════════════════════════════════════════
void handleSystemStates() {
  switch (currentState) {
    case STATE_IDLE: {
      // Hard lock rule: if not playing/recording, we can act.
      // 1) If button is held + tag present -> wipe (only when not playing)
      bool held = (digitalRead(BUTTON_PIN) == LOW);
      if (rfidTagPresent && held) {
        wipeRFIDDataForUID(currentRFIDUID); // ignored during playback by design
        break;
      }

      // 2) Known tag & not yet played this presence → PLAY
      if (rfidTagPresent && audioFileExistsForRFID(currentRFIDUID) && !playedThisPresence) {
        playedThisPresence = true;          // one-time per presence
        presenceUID = currentRFIDUID;
        currentState = STATE_PLAYING;       // set state BEFORE calling
        playbackAudioForRFID(currentRFIDUID);
        // playback function sets currentState = STATE_IDLE at EOF
        break;
      }

      // 3) Unknown tag & not holding button → RECORD
      if (rfidTagPresent && !audioFileExistsForRFID(currentRFIDUID) && !held) {
        recordAudioForRFID(currentRFIDUID);
        currentState = STATE_RECORDING;
        recordingStartTime = millis();
      }
    } break;

    case STATE_RECORDING:
      if (millis() - recordingStartTime >= (RECORD_SECONDS * 1000UL)) {
        Serial.println(F("[✅] Recording completed"));
        currentState = STATE_IDLE;
      }
      break;

    case STATE_PLAYING:
      // No-op here; playback runs to EOF inside playbackAudioForRFID()
      break;

    default:
      currentState = STATE_IDLE;
      break;
  }
}

// ══════════════════════════════════════════════════════════════
// 📡 RFID (disabled during PLAYING/RECORDING by early returns)
// ══════════════════════════════════════════════════════════════
void checkRFIDTag() {
  // Enforce lock: do not process RFID during playback or while SD busy or recording
  if (currentState == STATE_PLAYING || currentState == STATE_RECORDING || fileSystemBusy) return;

  switchToRFIDSPI();

  bool cardDetectedNow = false;
  uint32_t startTime = millis();
  while (millis() - startTime < 30) {
    if (mfrc522.PICC_IsNewCardPresent()) {
      if (mfrc522.PICC_ReadCardSerial()) { cardDetectedNow = true; break; }
    }
    delayMicroseconds(500);
  }

  if (cardDetectedNow) {
    String newUID = rfidUIDToString(&mfrc522.uid);

    if (!rfidTagPresent || newUID != currentRFIDUID) {
      currentRFIDUID = newUID;
      rfidTagPresent = true;
      rfidStillDetecting = true;
      rfidFirstDetected = millis();
      lastRFIDDetected = millis();

      // New presence starts → clear per-presence gate
      presenceUID = currentRFIDUID;
      playedThisPresence = false;

      Serial.print(F("[📡] RFID DETECTED: ")); Serial.println(currentRFIDUID);
    } else {
      lastRFIDDetected = millis();
      rfidStillDetecting = true;
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

  } else {
    if (rfidTagPresent || rfidStillDetecting) {
      unsigned long timeSinceLast = millis() - lastRFIDDetected;
      unsigned long timeSinceFirst = millis() - rfidFirstDetected;

      if (timeSinceLast > RFID_DEBOUNCE_MS && timeSinceFirst > RFID_DETECTION_WINDOW) {
        // Presence ends → allow replay next time
        rfidTagPresent = false;
        rfidStillDetecting = false;
        currentRFIDUID = "";
        presenceUID = "";
        playedThisPresence = false;
      } else if (timeSinceLast > 1000) {
        // temporary drop; treat as not currently present but keep window
        rfidTagPresent = false;
      }
    }
  }
}

// ══════════════════════════════════════════════════════════════
// 🔘 BUTTON (status only + wipe re-arm not needed due to hard lock)
// ══════════════════════════════════════════════════════════════
void checkButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState != lastButtonState) {
    lastButtonChange = millis();
  }

  if ((millis() - lastButtonChange) > BUTTON_DEBOUNCE_MS) {
    if (currentButtonState == LOW && lastButtonState == HIGH) {
      Serial.println(F("[🔘] BUTTON PRESSED"));
    }
  }

  lastButtonState = currentButtonState;
}

// ══════════════════════════════════════════════════════════════
// 🧹 Wipe helper — REFUSES during PLAYBACK
// ══════════════════════════════════════════════════════════════
void wipeRFIDDataForUID(const String& rfidUID) {
  if (currentState == STATE_PLAYING) {
    Serial.println(F("[⏳] Playback in progress — wipe ignored until playback completes"));
    return;
  }
  if (!waitForFileSystemReady(1000)) {
    Serial.println(F("[❌] Cannot wipe: file system busy"));
    return;
  }

  switchToSDSPI();
  String filename = getFileNameForRFID(rfidUID);

  if (SD.exists(filename)) {
    Serial.print(F("[🗑️] Deleting file: ")); Serial.println(filename);
    if (SD.remove(filename)) {
      Serial.println(F("[✅] Tag data wiped"));
      // If we just wiped the file in current presence, reset gate so next presence will record/play as new
      playedThisPresence = false;
    } else {
      Serial.println(F("[❌] Failed to delete file"));
    }
  } else {
    Serial.print(F("[ℹ️] No data found for UID ")); Serial.println(rfidUID);
  }

  // FAT quirk: try uppercase variant too
  String alt = filename; alt.toUpperCase();
  if (alt != filename && SD.exists(alt)) SD.remove(alt);
}

// ══════════════════════════════════════════════════════════════
// 🚌 SPI BUS
// ══════════════════════════════════════════════════════════════
inline void csAllHigh() {
  digitalWrite(SD_CS_PIN, HIGH);
  digitalWrite(RFID_SS_PIN, HIGH);
}

void switchSPIBus(SPIBusState newBus) {
  if (currentSPIBus == newBus) return;

  csAllHigh();                // ensure no device is selected
  SPI.end();
  delayMicroseconds(SPI_SWITCH_DELAY_US);

  switch (newBus) {
    case SPI_BUS_SD:
      SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
      SPI.setFrequency(SD_SPI_FREQ);
      digitalWrite(SD_CS_PIN, HIGH);
      break;
    case SPI_BUS_RFID:
      SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
      SPI.setFrequency(RFID_SPI_FREQ);
      digitalWrite(RFID_SS_PIN, HIGH);
      break;
    case SPI_BUS_NONE:
      break;
  }

  currentSPIBus = newBus;
  delayMicroseconds(SPI_SWITCH_DELAY_US);
}

void switchToSDSPI()   { switchSPIBus(SPI_BUS_SD); }
void switchToRFIDSPI() { switchSPIBus(SPI_BUS_RFID); }

// ══════════════════════════════════════════════════════════════
// 🎙️ RECORDING (self-contained; sets/clears FS busy; validates WAV)
// ══════════════════════════════════════════════════════════════
void recordAudioForRFID(const String& rfidUID) {
  if (currentState == STATE_PLAYING) { Serial.println(F("[⏳] Playing — record ignored")); return; }
  if (!waitForFileSystemReady(1000)) { Serial.println(F("[❌] File system not ready")); return; }
  if (isMemoryLow()) { Serial.println(F("[❌] Low memory — record aborted")); return; }

  fileSystemBusy = true;

  String filename = getFileNameForRFID(rfidUID);
  filename.toCharArray(currentWavFilename, sizeof(currentWavFilename));

  switchToSDSPI();
  if (SD.exists(currentWavFilename)) {
    Serial.println(F("[🗑️] Removing existing file"));
    SD.remove(currentWavFilename);
  }

  audioFile = SD.open(currentWavFilename, FILE_WRITE);
  if (!audioFile) {
    Serial.println(F("[❌] Failed to create audio file"));
    fileSystemBusy = false;
    return;
  }

  Serial.print(F("[📝] File created: ")); Serial.println(currentWavFilename);
  writeWAVHeader(audioFile);

  uint32_t bytesRecorded = 0;
  uint8_t buffer[I2S_BUFFER_SIZE];
  unsigned long startTime = millis();
  Serial.print(F("[🎙️] Recording ")); Serial.print(RECORD_SECONDS); Serial.print(F("s for UID ")); Serial.println(rfidUID);
  Serial.print(F("[📊] Progress: "));

  while (millis() - startTime < (RECORD_SECONDS * 1000UL)) {
    size_t bytesRead = 0;
    esp_err_t result = i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, 50);
    if (result == ESP_OK && bytesRead > 0) {
      size_t bytesWritten = audioFile.write(buffer, bytesRead);
      if (bytesWritten != bytesRead) { Serial.println(F("\n[❌] SD write error")); break; }
      bytesRecorded += bytesRead;
    } else if (result != ESP_OK) {
      Serial.print(F("\n[⚠️] I2S read error: ")); Serial.println(result);
    }
    if ((millis() - startTime) % 3000 < 100) Serial.print(F("●"));
    if ((millis() - startTime) % 1000 == 0) yield();
  }
  Serial.println();

  updateWAVHeader(audioFile, bytesRecorded);
  audioFile.close();
  fileSystemBusy = false;

  Serial.print(F("[💾] Saved: ")); Serial.print(currentWavFilename); Serial.print(F(" ("));
  Serial.print(bytesRecorded / 1024); Serial.println(F(" KB)"));

  if (validateWAVFile(currentWavFilename)) Serial.println(F("[✅] WAV validation OK"));
  else { Serial.println(F("[❌] WAV validation failed")); printWAVFileInfo(currentWavFilename); }
}

// ══════════════════════════════════════════════════════════════
// 🔊 PLAYBACK — play to EOF, ignore tag removal, hard lock enforced
// ══════════════════════════════════════════════════════════════
void playbackAudioForRFID(const String& rfidUID) {
  if (!waitForFileSystemReady(1000)) { Serial.println(F("[❌] File system not ready")); currentState = STATE_IDLE; return; }
  fileSystemBusy = true;

  String filename = getFileNameForRFID(rfidUID);
  switchToSDSPI();

  File playFile = SD.open(filename, FILE_READ);
  if (!playFile) {
    Serial.print(F("[❌] File not found: ")); Serial.println(filename);
    fileSystemBusy = false; currentState = STATE_IDLE; return;
  }

  if (!initI2SPlayback()) {
    playFile.close();
    fileSystemBusy = false;
    Serial.println(F("[❌] I2S playback failed"));
    currentState = STATE_IDLE;
    return;
  }

  playFile.seek(sizeof(WAVHeader));
  Serial.print(F("[🔊] Playing for UID ")); Serial.println(rfidUID);

  uint8_t txBuffer[I2S_BUFFER_SIZE];
  size_t bytesRead = 0;

  // Play to EOF (no state guard here)
  while ((bytesRead = playFile.read(txBuffer, sizeof(txBuffer))) > 0) {
    int16_t* samples = (int16_t*)txBuffer;
    for (size_t i = 0; i < bytesRead / 2; i++) samples[i] = (int16_t)(samples[i] * volume);
    size_t bytesWritten = 0;
    i2s_write(I2S_NUM_1, txBuffer, bytesRead, &bytesWritten, 100);
    yield();
  }

  playFile.close();
  cleanupI2SPlayback();
  fileSystemBusy = false;
  Serial.println(F("[✅] Playback completed"));

  currentState = STATE_IDLE;
}

// ══════════════════════════════════════════════════════════════
// 🔧 INIT
// ══════════════════════════════════════════════════════════════
bool initSDCard() {
  Serial.print(F("[💾] Initializing SD... "));
  switchToSDSPI();
  for (int attempt = 1; attempt <= 3; attempt++) {
    if (SD.begin(SD_CS_PIN, SPI, SD_SPI_FREQ)) {
      uint64_t cardSize = SD.cardSize() / (1024ULL * 1024ULL);
      if (cardSize > 0) { Serial.print(F("OK (")); Serial.print(cardSize); Serial.println(F(" MB)")); return true; }
    }
    delay(100);
  }
  Serial.println(F("FAILED"));
  return false;
}

bool initRFID() {
  Serial.print(F("[📡] Initializing RFID... "));
  switchToRFIDSPI();
  for (int attempt = 1; attempt <= 3; attempt++) {
    mfrc522.PCD_Init();
    delay(100);
    byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
    if (version != 0x00 && version != 0xFF) {
      Serial.print(F("OK (v0x")); Serial.print(version, HEX); Serial.println(F(")"));
      return true;
    }
    delay(100);
  }
  Serial.println(F("FAILED"));
  return false;
}

bool initI2SRecording() {
  Serial.print(F("[🎙️] Initializing I2S RX... "));
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = I2S_DMA_BUFFERS,
    .dma_buf_len = 64,
    .use_apll = false
  };
  if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) { Serial.println(F("FAILED")); return false; }
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S0_SCK_PIN,
    .ws_io_num = I2S0_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S0_RX_DATA_PIN
  };
  if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) { Serial.println(F("FAILED")); return false; }
  Serial.println(F("OK"));
  return true;
}

bool initI2SPlayback() {
  cleanupI2SPlayback();
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = I2S_DMA_BUFFERS,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };
  if (i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL) != ESP_OK) return false;
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S1_SCK_PIN,
    .ws_io_num = I2S1_WS_PIN,
    .data_out_num = I2S1_TX_DATA_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  return (i2s_set_pin(I2S_NUM_1, &pin_config) == ESP_OK);
}

void cleanupI2SPlayback() {
  i2s_driver_uninstall(I2S_NUM_1);
}

// ══════════════════════════════════════════════════════════════
// 📁 WAV HELPERS
// ══════════════════════════════════════════════════════════════
void writeWAVHeader(File &file) {
  WAVHeader header; memset(&header, 0, sizeof(header));
  memcpy(header.chunkID, "RIFF", 4);
  header.chunkSize = 0;
  memcpy(header.format, "WAVE", 4);
  memcpy(header.subchunk1ID, "fmt ", 4);
  header.subchunk1Size = 16;
  header.audioFormat = 1;
  header.numChannels = CHANNELS;
  header.sampleRate = SAMPLE_RATE;
  header.bitsPerSample = 16;
  header.byteRate = SAMPLE_RATE * CHANNELS * (16 / 8);
  header.blockAlign = CHANNELS * (16 / 8);
  memcpy(header.subchunk2ID, "data", 4);
  header.subchunk2Size = 0;

  size_t bytesWritten = file.write((uint8_t*)&header, sizeof(header));
  file.flush();

  Serial.print(F("[📝] WAV header bytes: ")); Serial.print(bytesWritten);
  Serial.print(F(" (expected ")); Serial.print(sizeof(header)); Serial.println(F(")"));
  if (bytesWritten == sizeof(header)) Serial.println(F("[✅] WAV header OK"));
  else Serial.println(F("[❌] WAV header write failed"));
}

void updateWAVHeader(File &file, uint32_t dataSize) {
  uint32_t fileSize = dataSize + sizeof(WAVHeader) - 8;
  file.seek(4);  file.write((uint8_t*)&fileSize, 4);
  file.seek(40); file.write((uint8_t*)&dataSize, 4);
  file.flush();
  Serial.println(F("[✅] WAV header updated"));
}

bool validateWAVFile(const String& filename) {
  if (fileSystemBusy) return false;
  switchToSDSPI();
  File wavFile = SD.open(filename, FILE_READ);
  if (!wavFile) { Serial.print(F("[❌] Cannot open for validation: ")); Serial.println(filename); return false; }
  WAVHeader header; size_t n = wavFile.read((uint8_t*)&header, sizeof(header));
  wavFile.close();
  if (n != sizeof(header)) return false;
  if (memcmp(header.chunkID, "RIFF", 4) != 0) return false;
  if (memcmp(header.format, "WAVE", 4) != 0) return false;
  if (memcmp(header.subchunk1ID, "fmt ", 4) != 0) return false;
  if (memcmp(header.subchunk2ID, "data", 4) != 0) return false;
  if (header.audioFormat != 1) return false;
  if (header.numChannels != CHANNELS) return false;
  if (header.sampleRate != SAMPLE_RATE) return false;
  return true;
}

void printWAVFileInfo(const String& filename) {
  if (fileSystemBusy) return;
  switchToSDSPI();
  File wavFile = SD.open(filename, FILE_READ);
  if (!wavFile) { Serial.println(F("[❌] Cannot open file")); return; }
  WAVHeader header; wavFile.read((uint8_t*)&header, sizeof(header)); wavFile.close();

  Serial.println(F("┌─────────────────────────────────────────────────────┐"));
  Serial.println(F("│                  WAV FILE INFO                      │"));
  Serial.println(F("├─────────────────────────────────────────────────────┤"));
  Serial.print(F("│ File: ")); Serial.println(filename);
  Serial.print(F("│ SR: ")); Serial.print(header.sampleRate); Serial.print(F("  Ch: ")); Serial.print(header.numChannels);
  Serial.print(F("  BPS: ")); Serial.println(header.bitsPerSample);
  Serial.print(F("│ File Size: ")); Serial.print(header.chunkSize + 8); Serial.println(F(" bytes"));
  Serial.print(F("│ Data Size: ")); Serial.print(header.subchunk2Size); Serial.println(F(" bytes"));
  Serial.println(F("└─────────────────────────────────────────────────────┘"));
}

void validateAllWAVFiles() {
  if (fileSystemBusy) { Serial.println(F("[❌] File system busy")); return; }
  switchToSDSPI();
  File root = SD.open("/");
  if (!root) { Serial.println(F("[❌] Cannot access SD")); return; }

  Serial.println(F("┌─────────────────────────────────────────────────────┐"));
  Serial.println(F("│                WAV FILE VALIDATION                  │"));
  Serial.println(F("├─────────────────────────────────────────────────────┤"));

  File entry = root.openNextFile();
  int count = 0, valid = 0;
  while (entry) {
    String name = entry.name();
    if (name.endsWith(".wav") || name.endsWith(".WAV")) {
      count++;
      entry.close();
      if (validateWAVFile("/" + name)) { Serial.print(F("│ ")); Serial.print(name); Serial.println(F(" ✅")); valid++; }
      else { Serial.print(F("│ ")); Serial.print(name); Serial.println(F(" ❌")); }
      entry = root.openNextFile();
    } else { entry.close(); entry = root.openNextFile(); }
  }
  Serial.println(F("├─────────────────────────────────────────────────────┤"));
  Serial.print(F("│ Results: ")); Serial.print(valid); Serial.print(F("/")); Serial.print(count); Serial.println(F(" valid │"));
  Serial.println(F("└─────────────────────────────────────────────────────┘"));
  root.close();
}

// ══════════════════════════════════════════════════════════════
// 💬 SERIAL — blocked during PLAYBACK
// ══════════════════════════════════════════════════════════════
void processSerialCommands() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n'); cmd.trim();

  if (currentState == STATE_PLAYING) {
    Serial.println(F("[⏳] Playing audio — command ignored until playback completes"));
    return;
  }

  if (cmd.equals("1")) {
    String filename = "/MANUAL_" + String(millis()) + ".wav";
    recordManualAudio(filename);
  } else if (cmd.equals("2")) {
    listSDFiles();
  } else if (cmd.equals("3")) {
    printSystemStatus();
  } else if (cmd.equals("4")) {
    validateAllWAVFiles();
  } else if (cmd.startsWith("info ")) {
    String filename = cmd.substring(5);
    printWAVFileInfo(filename);
  } else if (cmd.startsWith("v")) {
    int vol = cmd.substring(1).toInt();
    if (vol >= 0 && vol <= 100) setVolume(vol);
    else Serial.println(F("[❌] Volume must be 0-100"));
  } else if (cmd.length() > 0) {
    Serial.println(F("[❌] Unknown command"));
    Serial.println(F("[💡] Commands: 1=record, 2=list, 3=status, 4=validate, info <f>, v<%>"));
  }
}

void recordManualAudio(const String& filename) {
  if (currentState == STATE_PLAYING) { Serial.println(F("[⏳] Playing — manual record ignored")); return; }
  if (fileSystemBusy || isMemoryLow()) { Serial.println(F("[❌] Cannot record: busy")); return; }
  Serial.println(F("[🎙️] Manual recording started"));
  filename.toCharArray(currentWavFilename, sizeof(currentWavFilename));
  fileSystemBusy = true;
  switchToSDSPI();
  if (SD.exists(currentWavFilename)) SD.remove(currentWavFilename);
  audioFile = SD.open(currentWavFilename, FILE_WRITE);
  if (!audioFile) { Serial.println(F("[❌] Failed to create file")); fileSystemBusy = false; return; }
  writeWAVHeader(audioFile);
  uint32_t bytesRecorded = 0; uint8_t buffer[I2S_BUFFER_SIZE];
  unsigned long start = millis();
  Serial.print(F("[📊] Progress: "));
  while (millis() - start < (RECORD_SECONDS * 1000UL)) {
    size_t bytesRead = 0;
    i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, 100);
    if (bytesRead > 0) { audioFile.write(buffer, bytesRead); bytesRecorded += bytesRead; }
    if ((millis() - start) % 3000 < 100) Serial.print(F("●"));
    if ((millis() - start) % 1000 == 0) yield();
  }
  Serial.println();
  updateWAVHeader(audioFile, bytesRecorded);
  audioFile.close();
  fileSystemBusy = false;
  Serial.print(F("[💾] Saved: ")); Serial.println(filename);
  if (validateWAVFile(filename)) Serial.println(F("[✅] WAV OK")); else { Serial.println(F("[❌] WAV invalid")); printWAVFileInfo(filename); }
}

void listSDFiles() {
  if (currentState == STATE_PLAYING) { Serial.println(F("[⏳] Playing — list ignored")); return; }
  if (fileSystemBusy) { Serial.println(F("[❌] File system busy")); return; }
  switchToSDSPI();
  File root = SD.open("/");
  if (!root) { Serial.println(F("[❌] Cannot access SD")); return; }

  Serial.println(F("┌─────────────────────────────────────────────────────┐"));
  Serial.println(F("│                   SD CARD FILES                     │"));
  Serial.println(F("├─────────────────────────────────────────────────────┤"));

  File entry = root.openNextFile();
  int count = 0; uint32_t total = 0;
  while (entry) {
    Serial.print(F("│ ")); Serial.print(entry.name());
    int nameLen = strlen(entry.name());
    for (int i = nameLen; i < 35; i++) Serial.print(F(" "));
    uint32_t sz = entry.size(); total += sz;
    Serial.print(F(" (")); Serial.print(sz / 1024); Serial.println(F(" KB) │"));
    entry.close(); entry = root.openNextFile(); count++;
  }
  if (count == 0) {
    Serial.println(F("│                 No files found                     │"));
  } else {
    Serial.println(F("├─────────────────────────────────────────────────────┤"));
    Serial.print(F("│ Total: ")); Serial.print(count); Serial.print(F(" files, ")); Serial.print(total / 1024); Serial.println(F(" KB                        │"));
  }
  Serial.println(F("└─────────────────────────────────────────────────────┘"));
  root.close();
}

void printSystemStatus() {
  Serial.println(F("┌─────────────────────────────────────────────────────┐"));
  Serial.println(F("│                  SYSTEM STATUS                      │"));
  Serial.println(F("├─────────────────────────────────────────────────────┤"));
  Serial.print(F("│ State: "));
  const char* states[] = {"IDLE", "RECORDING", "PLAYING", "ERROR"};
  Serial.println(states[currentState]);
  Serial.print(F("│ RFID: "));
  if (rfidTagPresent) { Serial.print(F("PRESENT (")); Serial.print(currentRFIDUID.substring(0, 8)); Serial.println(F("...)")); }
  else if (rfidStillDetecting) { Serial.println(F("WINDOW ACTIVE")); }
  else { Serial.println(F("NO TAG")); }
  Serial.print(F("│ Volume: ")); Serial.print((int)(volume * 100)); Serial.println(F("%"));
  Serial.print(F("│ Free RAM: ")); Serial.print(ESP.getFreeHeap() / 1024); Serial.println(F(" KB"));
  Serial.print(F("│ Button Status: ")); Serial.println((digitalRead(BUTTON_PIN) == LOW) ? F("PRESSED") : F("RELEASED"));
  Serial.print(F("│ WAV Header Size: ")); Serial.print(sizeof(WAVHeader)); Serial.println(F(" bytes"));
  Serial.println(F("└─────────────────────────────────────────────────────┘"));
}

// ══════════════════════════════════════════════════════════════
// 🛡️ UTIL
// ══════════════════════════════════════════════════════════════
void feedWatchdog() { yield(); }
bool isMemoryLow() { return (ESP.getFreeHeap() < 30000); }
bool waitForFileSystemReady(uint32_t timeoutMs) {
  uint32_t start = millis();
  while (fileSystemBusy && (millis() - start < timeoutMs)) delay(10);
  return !fileSystemBusy;
}

// ══════════════════════════════════════════════════════════════
// 🔧 MISC
// ══════════════════════════════════════════════════════════════
String rfidUIDToString(MFRC522::Uid *uid) {
  String s = "";
  for (byte i = 0; i < uid->size; i++) {
    if (uid->uidByte[i] < 0x10) s += "0";
    s += String(uid->uidByte[i], HEX);
  }
  s.toUpperCase();
  return s;
}

String getFileNameForRFID(const String& rfidUID) {
  return String("/") + "RFID_" + rfidUID + ".wav";
}

bool audioFileExistsForRFID(const String& rfidUID) {
  if (fileSystemBusy) return false;
  switchToSDSPI();
  return SD.exists(getFileNameForRFID(rfidUID));
}

void setVolume(int volPercent) {
  volume = constrain(volPercent, 0, 100) / 100.0f;
  Serial.print(F("[🔊] Volume set to ")); Serial.print(volPercent); Serial.println(F("%"));
}
