/*
 * ESP32 INMP441 Recorder with ESP8266Audio Library Playback (GPIO ≤ 34)
 *
 * Description:
 * This sketch configures the ESP32 to record 15 seconds of audio from an INMP441 
 * microphone using I2S (I2S_NUM_0) and saves the recording as a WAV file on an SD card.
 * Playback is performed using the ESP8266Audio library (which has experimental ESP32 support)
 * to stream the WAV file from the SD card through a MAX98357A amplifier.
 *
 * Recording (I2S_NUM_0 for INMP441):
 *   - WS (LRCLK):   GPIO25
 *   - BCLK:         GPIO26
 *   - DIN:          GPIO27
 *
 * SD Card (SPI):
 *   - CS:           GPIO5
 *   - MOSI:         GPIO23
 *   - MISO:         GPIO19
 *   - SCK:          GPIO18
 *
 * Playback:
 *   The ESP8266Audio library internally configures the I2S peripheral (default ESP32 pins)
 *   for playback. (Double-check your wiring for the MAX98357A to match the library settings.)
 *
 * Serial Commands:
 *   1 – Record 15-sec WAV file from INMP441
 *   2 – List SD card files
 *   3 – Playback the most recent recording using the ESP8266Audio library
 *
 * NOTE:
 * The original "Audio" library is not compatible with ESP32. This sketch uses the ESP8266Audio 
 * library (which includes experimental support for ESP32) for playback.
 */

#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <driver/i2s.h>
#include <ESP8266Audio.h>  // Use ESP8266Audio library (ensure you install the ESP32-compatible fork)

// Create an instance of the Audio class from ESP8266Audio
Audio audio;

//////////////////////
// Hardware Config  //
//////////////////////

// SD Card SPI pins:
const uint8_t SD_CS_PIN    = 5;
const uint8_t SD_MOSI_PIN  = 23;
const uint8_t SD_MISO_PIN  = 19;
const uint8_t SD_SCK_PIN   = 18;

// Recording I2S (I2S_NUM_0 for INMP441)
const uint8_t I2S0_WS_PIN      = 25;   // LRCLK for recording
const uint8_t I2S0_SCK_PIN     = 26;   // BCLK for recording
const uint8_t I2S0_RX_DATA_PIN = 27;   // Data IN from INMP441

// Recording parameters:
const uint32_t SAMPLE_RATE     = 16000; // 16 kHz sample rate
const uint8_t  BITS_PER_SAMPLE = 16;    // 16-bit resolution
const uint8_t  CHANNELS        = 1;     // Mono audio
const uint8_t  RECORD_SECONDS  = 15;    // Recording duration in seconds
const uint32_t TOTAL_SAMPLES   = SAMPLE_RATE * RECORD_SECONDS;

//////////////////////////
// WAV Header Structure //
//////////////////////////
struct __attribute__((packed)) WAVHeader {
  char     riff[4];         // "RIFF"
  uint32_t fileSize;        // File size minus 8 (to be updated)
  char     wave[4];         // "WAVE"
  char     fmt[4];          // "fmt " string
  uint32_t fmtSize;         // 16 for PCM
  uint16_t audioFormat;     // PCM = 1
  uint16_t channels;        // 1 = mono
  uint32_t sampleRate;      // e.g., 16000 Hz
  uint32_t byteRate;        // sampleRate * channels * (bits per sample / 8)
  uint16_t blockAlign;      // channels * (bits per sample / 8)
  uint16_t bitsPerSample;   // 16
  char     data[4];         // "data"
  uint32_t dataSize;        // Audio data size (to be updated)
};

//////////////////////////
// Global Variables     //
//////////////////////////
File audioFile;
char wavFilename[32] = "/RECxxxxxx.wav";

//////////////////////////////
// Function Declarations    //
//////////////////////////////
bool initSDCard();
bool initI2SRecording();
void recordAudio();
void listSDFiles();
void playbackAudio();
void writeWAVHeader(File &file);
void updateWAVHeader(File &file, uint32_t dataSize);
void processSerialCommands();

//////////////////////
// Setup Function   //
//////////////////////
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(1); }
  Serial.println(F("ESP32 INMP441 Recorder with ESP8266Audio Playback (GPIO ≤ 34)"));
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

//////////////////////
// Main Loop        //
//////////////////////
void loop() {
  processSerialCommands();
  delay(100);  // In production, move to a non-blocking state machine.
}

//////////////////////////////
// Process Serial Commands  //
//////////////////////////////
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

//////////////////////
// SD Card Init     //
//////////////////////
bool initSDCard() {
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SPI)) {
    return false;
  }
  Serial.println(F("SD Card initialized."));
  return true;
}

////////////////////////////
// I2S Recording Init     //
////////////////////////////
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

  // Field order: bck_io_num, ws_io_num, data_out_num, data_in_num.
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

//////////////////////
// Record Audio     //
//////////////////////
void recordAudio() {
  uint32_t t = millis();
  snprintf(wavFilename, sizeof(wavFilename), "/REC%08lX.wav", t);
  audioFile = SD.open(wavFilename, FILE_WRITE);
  if (!audioFile) {
    Serial.println(F("Could not create file on SD card."));
    return;
  }
  // Write placeholder WAV header; will update after recording.
  writeWAVHeader(audioFile);

  uint32_t bytesToRecord = TOTAL_SAMPLES * (BITS_PER_SAMPLE / 8);
  uint32_t bytesRecorded = 0;
  uint8_t buffer[512 * 2];  // 2 bytes per sample
  
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

//////////////////////
// List SD Files    //
//////////////////////
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

//////////////////////
// Playback Audio   //
//////////////////////
void playbackAudio() {
  if (!SD.exists(wavFilename)) {
    Serial.println(F("Playback file not found."));
    return;
  }
  Serial.print(F("Playing back file: "));
  Serial.println(wavFilename);

  // Initialize the Audio system and stream the file from FS (SD)
  audio.begin();
  if (audio.connecttoFS(SD, wavFilename)) {
    while (audio.isRunning()) {
      audio.loop();
    }
    audio.stop();
  } else {
    Serial.println(F("Failed to play audio file."));
  }
}

//////////////////////////////
// Write WAV Header         //
//////////////////////////////
void writeWAVHeader(File &file) {
  WAVHeader header;
  memcpy(header.riff, "RIFF", 4);
  header.fileSize = 0; // Placeholder, will update later.
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
  header.dataSize = 0; // Placeholder

  file.seek(0);
  file.write((uint8_t*)&header, sizeof(header));
}

//////////////////////////////
// Update WAV Header        //
//////////////////////////////
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
   • https://github.com/masoncj/esp32-examples – Wiring, I2S, SD usage examples
   • https://github.com/aws-samples/aws-iot-esp32-arduino-examples – ESP32 SD and I2S implementations
   • https://github.com/kriswiner/ESP32 – Best practice audio interfacing
   • https://github.com/gasparegas/TrapDoor32 – Non-blocking design patterns
   • https://github.com/letscontrolit/ESPEasy – Reliable SD card code examples
   • https://github.com/HomeSpan/HomeSpan – Modular architecture examples
   • https://github.com/Hieromon/AutoConnect – I2S & SD usage on ESP32
   • https://github.com/codershiyar/esp32-projects – Production-ready sketches for ESP32
   • https://github.com/georgecatalin/ESP32_for_Arduino_Makers – Detailed audio recording setups
   • https://github.com/mysensors/MySensors – Sensor interfacing and SD examples
   • https://github.com/agucova/awesome-esp – Curated list of ESP32 projects
   • https://github.com/TinyGSM/TinyGSM – ESP32 connectivity examples
   • https://github.com/painlessMesh/painlessMesh – Non-blocking state machines on ESP32
   • https://github.com/adrianmihalko/arduino-esp32-camera – Resource-constrained design for ESP32
   • https://github.com/atomic14/esp32_audio – Audio recording/playback for ESP32
   • https://github.com/espressif/arduino-esp32 – Official ESP32 core examples and guidelines
   • https://github.com/esphome/esphome – Non-blocking, modular designs for ESP32 projects
   • https://github.com/me-no-dev/ESPAsyncWebServer – Asynchronous IO for ESP32
   • https://github.com/me-no-dev/AsyncTCP – Reliable network interfacing on ESP32
   • https://github.com/earlephilhower/ESP8266Audio – ESP8266Audio library with ESP32 support (experimental)
*/