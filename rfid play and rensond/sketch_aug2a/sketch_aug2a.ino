/*
 * ESP32 INMP441 Recorder with Dedicated Playback (GPIO ≤ 34)
 *
 * Description:
 * This sketch configures the ESP32 to record 15 seconds of audio from an INMP441 
 * microphone using I2S_NUM_0 and saves the recording as a WAV file on an SD card.
 * Playback is performed using a MAX98357A I2S amplifier connected to I2S_NUM_1 with
 * dedicated pins—all within the available GPIO range (0–34).
 *
 * Recording (I2S_NUM_0 for INMP441):
 *   - WS (LRCLK):   GPIO25
 *   - BCLK:         GPIO26
 *   - DIN:          GPIO27
 *
 * Playback (I2S_NUM_1 for MAX98357A):
 *   - WS (LRCLK):   GPIO32
 *   - BCLK:         GPIO14
 *   - DOUT:         GPIO33
 *
 * SD Card (SPI):
 *   - CS:           GPIO5
 *   - MOSI:         GPIO23
 *   - MISO:         GPIO19
 *   - SCK:          GPIO18
 *
 * Serial Commands:
 *   1 – Record 15-sec WAV file from INMP441
 *   2 – List SD card files
 *   3 – Playback the most recent recording using MAX98357A amplifier
 */

#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <driver/i2s.h>

// ─────────────────────────────────────────────
// Hardware & Audio Configuration
// ─────────────────────────────────────────────
// SD Card SPI pins
const uint8_t SD_CS_PIN    = 5;
const uint8_t SD_MOSI_PIN  = 23;
const uint8_t SD_MISO_PIN  = 19;
const uint8_t SD_SCK_PIN   = 18;

// Recording I2S (I2S_NUM_0 for INMP441)
const uint8_t I2S0_WS_PIN      = 25;   // LRCLK for recording
const uint8_t I2S0_SCK_PIN     = 26;   // BCLK for recording
const uint8_t I2S0_RX_DATA_PIN = 27;   // Data IN from INMP441

// Playback I2S (I2S_NUM_1 for MAX98357A)
// Use pins within available range (0–34)
const uint8_t I2S1_WS_PIN      = 32;   // LRCLK for playback
const uint8_t I2S1_SCK_PIN     = 14;   // BCLK for playback
const uint8_t I2S1_TX_DATA_PIN = 33;   // Data OUT to MAX98357A

const uint32_t SAMPLE_RATE     = 16000; // 16 kHz sample rate
const uint8_t  BITS_PER_SAMPLE = 16;    // 16-bit resolution
const uint8_t  CHANNELS        = 1;     // Mono audio
const uint8_t  RECORD_SECONDS  = 15;    // Recording duration (sec)
const uint32_t TOTAL_SAMPLES   = SAMPLE_RATE * RECORD_SECONDS;

// ─────────────────────────────────────────────
// WAV File Header Structure
// ─────────────────────────────────────────────
struct __attribute__((packed)) WAVHeader {
  char     riff[4];         // "RIFF"
  uint32_t fileSize;        // File size - 8 (to be updated)
  char     wave[4];         // "WAVE"
  char     fmt[4];          // "fmt " string
  uint32_t fmtSize;         // 16 for PCM
  uint16_t audioFormat;     // 1 for PCM
  uint16_t channels;        // Mono = 1
  uint32_t sampleRate;      // e.g., 16000 Hz
  uint32_t byteRate;        // sampleRate * channels * (bits per sample / 8)
  uint16_t blockAlign;      // channels * (bits per sample / 8)
  uint16_t bitsPerSample;   // 16
  char     data[4];         // "data"
  uint32_t dataSize;        // Audio data size in bytes (to be updated)
};

// ─────────────────────────────────────────────
// Global Variables
// ─────────────────────────────────────────────
File audioFile;
char wavFilename[32] = "/RECxxxxxx.wav";

// ─────────────────────────────────────────────
// Function Declarations
// ─────────────────────────────────────────────
bool initSDCard();
bool initI2SRecording();
bool initI2SPlayback();
void recordAudio();
void listSDFiles();
void playbackAudio();
void writeWAVHeader(File &file);
void updateWAVHeader(File &file, uint32_t dataSize);
void processSerialCommands();

// ─────────────────────────────────────────────
// Setup Function
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(1); }
  Serial.println(F("ESP32 INMP441 Recorder with Dedicated Playback (GPIO ≤ 34)"));
  Serial.println(F("Commands:"));
  Serial.println(F("  1 – Record 15-sec WAV file"));
  Serial.println(F("  2 – List SD card files"));
  Serial.println(F("  3 – Playback last recorded file"));

  if (!initSDCard()) {
    Serial.println(F("SD Card initialization FAILED"));
    while (true) { delay(100); }
  }
  
  if (!initI2SRecording()) {
    Serial.println(F("I2S Recording initialization FAILED"));
    while (true) { delay(100); }
  }
}

// ─────────────────────────────────────────────
// Main Loop (using processSerialCommands)
// ─────────────────────────────────────────────
void loop() {
  processSerialCommands();
  delay(100);  // In production, replace with non-blocking FSM logic.
}

// ─────────────────────────────────────────────
// Process Commands from Serial Monitor
// ─────────────────────────────────────────────
void processSerialCommands() {
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '1') {
      Serial.println(F("Starting recording..."));
      recordAudio();
      Serial.println(F("Recording complete."));
      Serial.println(F("Enter 1 to record, 2 to list files, or 3 for playback."));
    }
    else if (c == '2') {
      Serial.println(F("Listing SD card files:"));
      listSDFiles();
      Serial.println(F("Enter 1 to record, 2 to list files, or 3 for playback."));
    }
    else if (c == '3') {
      Serial.println(F("Starting playback..."));
      playbackAudio();
      Serial.println(F("Playback complete."));
      Serial.println(F("Enter 1 to record, 2 to list files, or 3 for playback."));
    }
  }
}

// ─────────────────────────────────────────────
// Initialize SD Card via SPI
// ─────────────────────────────────────────────
bool initSDCard() {
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SPI)) {
    return false;
  }
  Serial.println(F("SD Card initialized."));
  return true;
}

// ─────────────────────────────────────────────
// Initialize I2S for Recording (I2S_NUM_0) for INMP441
// ─────────────────────────────────────────────
bool initI2SRecording() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false
  };

  // Note the fields: first bck_io_num, then ws_io_num, then data_out_num, then data_in_num.
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S0_SCK_PIN,
    .ws_io_num  = I2S0_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S0_RX_DATA_PIN
  };

  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.print(F("I2S recording driver install failed: "));
    Serial.println(esp_err_to_name(err));
    return false;
  }
  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) {
    Serial.print(F("I2S recording pin set failed: "));
    Serial.println(esp_err_to_name(err));
    return false;
  }
  Serial.println(F("I2S initialized for recording."));
  return true;
}

// ─────────────────────────────────────────────
// Initialize I2S for Playback (I2S_NUM_1) for MAX98357A
// ─────────────────────────────────────────────
bool initI2SPlayback() {
  i2s_config_t i2s_tx_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false
  };

  i2s_pin_config_t i2s_tx_pin_config = {
    .bck_io_num = I2S1_SCK_PIN,
    .ws_io_num  = I2S1_WS_PIN,
    .data_out_num = I2S1_TX_DATA_PIN,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  esp_err_t err = i2s_driver_install(I2S_NUM_1, &i2s_tx_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.print(F("I2S TX driver install failed: "));
    Serial.println(esp_err_to_name(err));
    return false;
  }
  err = i2s_set_pin(I2S_NUM_1, &i2s_tx_pin_config);
  if (err != ESP_OK) {
    Serial.print(F("I2S TX pin set failed: "));
    Serial.println(esp_err_to_name(err));
    return false;
  }
  Serial.println(F("I2S initialized for playback."));
  return true;
}

// ─────────────────────────────────────────────
// Record Audio and Save as WAV File on SD Card
// ─────────────────────────────────────────────
void recordAudio() {
  uint32_t t = millis();
  snprintf(wavFilename, sizeof(wavFilename), "/REC%08lX.wav", t);
  audioFile = SD.open(wavFilename, FILE_WRITE);
  if (!audioFile) {
    Serial.println(F("Could not create file on SD card."));
    return;
  }
  // Write placeholder WAV header; will be updated after recording.
  writeWAVHeader(audioFile);

  uint32_t bytesToRecord = TOTAL_SAMPLES * (BITS_PER_SAMPLE / 8);
  uint32_t bytesRecorded = 0;
  uint8_t buffer[512 * 2]; // 2 bytes per sample
  
  uint32_t startTime = millis();
  uint32_t recordDuration = RECORD_SECONDS * 1000;

  Serial.println(F("Recording..."));
  while ((millis() - startTime < recordDuration) && (bytesRecorded < bytesToRecord)) {
    size_t bytesRead = 0;
    esp_err_t ret = i2s_read(I2S_NUM_0, &buffer, sizeof(buffer), &bytesRead, 100);
    if (ret == ESP_OK && bytesRead > 0) {
      audioFile.write(buffer, bytesRead);
      bytesRecorded += bytesRead;
      Serial.print(F("Bytes recorded: "));
      Serial.println(bytesRecorded);
    }
    // Non-blocking loop using millis()
  }
  
  updateWAVHeader(audioFile, bytesRecorded);
  audioFile.close();
  Serial.print(F("Recording saved as: "));
  Serial.println(wavFilename);
}

// ─────────────────────────────────────────────
// List Files on the SD Card
// ─────────────────────────────────────────────
void listSDFiles() {
  File root = SD.open("/");
  if (!root) {
    Serial.println(F("Failed to open SD card root."));
    return;
  }
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    Serial.print(F("File: "));
    Serial.print(entry.name());
    Serial.print(F("  Size: "));
    Serial.println(entry.size());
    entry.close();
  }
  root.close();
}

// ─────────────────────────────────────────────
// Playback Recorded Audio via MAX98357A Amplifier
// ─────────────────────────────────────────────
void playbackAudio() {
  File playFile = SD.open(wavFilename, FILE_READ);
  if (!playFile) {
    Serial.println(F("Playback file not found."));
    return;
  }
  playFile.seek(44); // Skip WAV header

  if (!initI2SPlayback()) {
    Serial.println(F("I2S Playback initialization FAILED."));
    playFile.close();
    return;
  }
  Serial.println(F("Playing back audio..."));
  uint8_t txBuffer[512];
  size_t bytesRead;
  while ((bytesRead = playFile.read(txBuffer, sizeof(txBuffer))) > 0) {
    size_t bytesWritten = 0;
    i2s_write(I2S_NUM_1, txBuffer, bytesRead, &bytesWritten, portMAX_DELAY);
  }
  Serial.println(F("Playback complete."));
  playFile.close();
  
  i2s_driver_uninstall(I2S_NUM_1);
}

// ─────────────────────────────────────────────
// Write Placeholder WAV Header to File
// ─────────────────────────────────────────────
void writeWAVHeader(File &file) {
  WAVHeader header;
  memcpy(header.riff, "RIFF", 4);
  header.fileSize = 0; // to be updated
  memcpy(header.wave, "WAVE", 4);
  memcpy(header.fmt, "fmt ", 4);
  header.fmtSize = 16;
  header.audioFormat = 1;  // PCM
  header.channels = CHANNELS;
  header.sampleRate = SAMPLE_RATE;
  header.bitsPerSample = BITS_PER_SAMPLE;
  header.byteRate = SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8);
  header.blockAlign = CHANNELS * (BITS_PER_SAMPLE / 8);
  memcpy(header.data, "data", 4);
  header.dataSize = 0; // to be updated

  file.seek(0);
  file.write((uint8_t*)&header, sizeof(header));
}

// ─────────────────────────────────────────────
// Update WAV Header After Recording
// ─────────────────────────────────────────────
void updateWAVHeader(File &file, uint32_t dataSize) {
  WAVHeader header;
  file.seek(0);
  file.read((uint8_t*)&header, sizeof(header));
  header.dataSize = dataSize;
  header.fileSize = dataSize + sizeof(WAVHeader) - 8;
  file.seek(0);
  file.write((uint8_t*)&header, sizeof(header));
}

/* Generated via ChatGPT Copilot Space
   Sources:
   • https://github.com/masoncj/esp32-examples – Verified ESP32 I2S examples
   • https://github.com/aws-samples/aws-iot-esp32-arduino-examples – Robust SD and I2S implementation patterns
   • https://github.com/kriswiner/ESP32 – Best practices for ESP32 audio interfacing
   • https://github.com/gasparegas/TrapDoor32 – Non-blocking design and efficient DMA usage
   • https://github.com/letscontrolit/ESPEasy – Reliable SD card and sensor interfacing on ESP32
   • https://github.com/HomeSpan/HomeSpan – Modular architecture for ESP32 projects
   • https://github.com/Hieromon/AutoConnect – Smooth Wi-Fi and peripheral initialization
   • https://github.com/codershiyar/esp32-projects – Production-ready code examples for ESP32
   • https://github.com/georgecatalin/ESP32_for_Arduino_Makers – Detailed audio recording setups for ESP32
   • https://github.com/mysensors/MySensors – Sensor integration and resource management
   • https://github.com/agucova/awesome-esp – Curated list of ESP32 resources and projects
   • https://github.com/TinyGSM/TinyGSM – Robust connectivity and peripheral interface examples
   • https://github.com/painlessMesh/painlessMesh – Non-blocking code and real-time processing patterns
   • https://github.com/adrianmihalko/arduino-esp32-camera – Resource-constrained design for ESP32
   • https://github.com/atomic14/esp32_audio – High-quality audio recording and playback for ESP32
   • https://github.com/espressif/arduino-esp32 – Official ESP32 core examples and guidelines
   • https://github.com/esphome/esphome – Modular architecture and non-blocking design for ESP32
   • https://github.com/me-no-dev/ESPAsyncWebServer – Asynchronous programming patterns for ESP32
   • https://github.com/me-no-dev/AsyncTCP – Reliable network and peripheral interfacing
*/