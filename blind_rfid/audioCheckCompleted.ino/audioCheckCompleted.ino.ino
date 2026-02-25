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
 *   v<num> – Set volume (0-100), e.g., v70 for 70% volume
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
const uint8_t I2S1_WS_PIN      = 32;   // LRCLK for playback
const uint8_t I2S1_SCK_PIN     = 14;   // BCLK for playback
const uint8_t I2S1_TX_DATA_PIN = 33;   // Data OUT to MAX98357A

const uint32_t SAMPLE_RATE     = 16000; // 16 kHz sample rate
const uint8_t  BITS_PER_SAMPLE = 16;    // 16-bit resolution
const uint8_t  CHANNELS        = 1;     // Mono audio
const uint8_t  RECORD_SECONDS  = 15;    // Recording duration (sec)

// ─────────────────────────────────────────────
// WAV File Header Structure
// ─────────────────────────────────────────────
struct __attribute__((packed)) WAVHeader {
  char     riff[4];
  uint32_t fileSize;
  char     wave[4];
  char     fmt[4];
  uint32_t fmtSize;
  uint16_t audioFormat;
  uint16_t channels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
  char     data[4];
  uint32_t dataSize;
};

// ─────────────────────────────────────────────
// Global Variables
// ─────────────────────────────────────────────
File audioFile;
char wavFilename[32] = "/norec.wav";
float volume = 1.0; // Volume multiplier (0.0 to 1.0)
unsigned long lastSerialCheck = 0;
const long serialCheckInterval = 100; // Check for serial commands every 100ms

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
void setVolume(int volPercent);

// ─────────────────────────────────────────────
// Setup Function
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(1); } // Wait for serial connection
  
  Serial.println(F("\nESP32 INMP441 Recorder/Player"));
  Serial.println(F("────────────────────────────────"));
  Serial.println(F("Commands:"));
  Serial.println(F("  1 – Record 15-sec WAV file"));
  Serial.println(F("  2 – List SD card files"));
  Serial.println(F("  3 – Playback last recorded file"));
  Serial.println(F("  v<num> – Set volume (0-100), e.g., v70"));
  Serial.println(F("────────────────────────────────"));

  if (!initSDCard()) {
    Serial.println(F("FATAL: SD Card initialization FAILED. Halting."));
    while (true) { delay(1000); }
  }
  
  if (!initI2SRecording()) {
    Serial.println(F("FATAL: I2S Recording initialization FAILED. Halting."));
    while (true) { delay(1000); }
  }
  
  setVolume(70); // Set default volume to 70%
}

// ─────────────────────────────────────────────
// Main Loop (Non-Blocking)
// ─────────────────────────────────────────────
void loop() {
  // Use a non-blocking timer to check for serial commands periodically
  if (millis() - lastSerialCheck >= serialCheckInterval) {
    processSerialCommands();
    lastSerialCheck = millis();
  }
  // The CPU is now free to handle other tasks
}

// ─────────────────────────────────────────────
// Process Commands from Serial Monitor (Robust)
// ─────────────────────────────────────────────
void processSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equals("1")) {
      Serial.println(F("COMMAND: Record Audio"));
      recordAudio();
      Serial.println(F("ACTION: Recording complete."));
    } else if (command.equals("2")) {
      Serial.println(F("COMMAND: List Files"));
      listSDFiles();
      Serial.println(F("ACTION: File list complete."));
    } else if (command.equals("3")) {
      Serial.println(F("COMMAND: Playback Audio"));
      playbackAudio();
      Serial.println(F("ACTION: Playback complete."));
    } else if (command.startsWith("v")) {
      Serial.println(F("COMMAND: Set Volume"));
      long volPercent = command.substring(1).toInt();
      if (volPercent >= 0 && volPercent <= 100) {
        setVolume(volPercent);
      } else {
        Serial.println(F("ERROR: Invalid volume. Use a value between 0 and 100."));
      }
    } else if (command.length() > 0) {
      Serial.print(F("ERROR: Unknown command '"));
      Serial.print(command);
      Serial.println(F("'"));
    }
  }
}

// ─────────────────────────────────────────────
// Set Volume
// ─────────────────────────────────────────────
void setVolume(int volPercent) {
    volume = constrain(volPercent, 0, 100) / 100.0f;
    Serial.print(F("INFO: Volume set to "));
    Serial.print(volPercent);
    Serial.println(F("%."));
}

// ─────────────────────────────────────────────
// Initialize SD Card via SPI
// ─────────────────────────────────────────────
bool initSDCard() {
  Serial.print(F("Initializing SD card... "));
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SPI)) {
    Serial.println(F("FAILED. Check wiring."));
    return false;
  }
  Serial.println(F("OK."));
  return true;
}

// ─────────────────────────────────────────────
// Initialize I2S for Recording (I2S_NUM_0)
// ─────────────────────────────────────────────
bool initI2SRecording() {
  Serial.print(F("Initializing I2S for Recording (I2S_NUM_0)... "));
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
    Serial.println(F("FAILED driver install."));
    return false;
  }
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S0_SCK_PIN,
    .ws_io_num  = I2S0_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S0_RX_DATA_PIN
  };

  if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) {
    Serial.println(F("FAILED pin set."));
    return false;
  }
  
  Serial.println(F("OK."));
  return true;
}

// ─────────────────────────────────────────────
// Initialize I2S for Playback (I2S_NUM_1)
// ─────────────────────────────────────────────
bool initI2SPlayback() {
  Serial.print(F("Initializing I2S for Playback (I2S_NUM_1)... "));
  i2s_driver_uninstall(I2S_NUM_1); // Ensure it's not already running
  
  i2s_config_t i2s_tx_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };

  if (i2s_driver_install(I2S_NUM_1, &i2s_tx_config, 0, NULL) != ESP_OK) {
    Serial.println(F("FAILED driver install."));
    return false;
  }
  
  i2s_pin_config_t i2s_tx_pin_config = {
    .bck_io_num = I2S1_SCK_PIN,
    .ws_io_num  = I2S1_WS_PIN,
    .data_out_num = I2S1_TX_DATA_PIN,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  if (i2s_set_pin(I2S_NUM_1, &i2s_tx_pin_config) != ESP_OK) {
    Serial.println(F("FAILED pin set."));
    return false;
  }
  
  Serial.println(F("OK."));
  return true;
}

// ─────────────────────────────────────────────
// Record Audio and Save as WAV File on SD Card
// ─────────────────────────────────────────────
void recordAudio() {
  snprintf(wavFilename, sizeof(wavFilename), "/REC%08lX.wav", (uint32_t)millis());
  audioFile = SD.open(wavFilename, FILE_WRITE);
  if (!audioFile) {
    Serial.println(F("ERROR: Could not create file on SD card."));
    return;
  }
  
  writeWAVHeader(audioFile);

  const uint32_t recordSizeBytes = SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8) * RECORD_SECONDS;
  uint32_t bytesRecorded = 0;
  uint8_t buffer[1024];
  
  Serial.print(F("INFO: Recording for "));
  Serial.print(RECORD_SECONDS);
  Serial.println(F(" seconds..."));
  
  unsigned long startTime = millis();
  while (millis() - startTime < (RECORD_SECONDS * 1000)) {
    size_t bytesRead = 0;
    i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);
    if (bytesRead > 0) {
      audioFile.write(buffer, bytesRead);
      bytesRecorded += bytesRead;
    }
  }
  
  updateWAVHeader(audioFile, bytesRecorded);
  audioFile.close();
  
  Serial.print(F("INFO: Recording saved as "));
  Serial.print(wavFilename);
  Serial.print(F(" ("));
  Serial.print(bytesRecorded / 1024);
  Serial.println(F(" KB)"));
}

// ─────────────────────────────────────────────
// List Files on the SD Card
// ─────────────────────────────────────────────
void listSDFiles() {
  File root = SD.open("/");
  if (!root) {
    Serial.println(F("ERROR: Failed to open SD card root directory."));
    return;
  }
  Serial.println(F("Files on SD Card:"));
  File entry = root.openNextFile();
  while (entry) {
    Serial.print(F("  - "));
    Serial.print(entry.name());
    Serial.print(F("\t ("));
    Serial.print(entry.size() / 1024);
    Serial.println(F(" KB)"));
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
}

// ─────────────────────────────────────────────
// Playback Recorded Audio via MAX98357A
// ─────────────────────────────────────────────
void playbackAudio() {
  if (strcmp(wavFilename, "/norec.wav") == 0) {
    Serial.println(F("ERROR: No file has been recorded yet. Record first with command '1'."));
    return;
  }
  
  File playFile = SD.open(wavFilename, FILE_READ);
  if (!playFile) {
    Serial.print(F("ERROR: Playback file not found: "));
    Serial.println(wavFilename);
    return;
  }

  if (!initI2SPlayback()) {
    playFile.close();
    return;
  }
  
  playFile.seek(sizeof(WAVHeader));
  Serial.println(F("INFO: Playing back audio..."));
  
  uint8_t txBuffer[1024];
  size_t bytesRead;
  
  while ((bytesRead = playFile.read(txBuffer, sizeof(txBuffer))) > 0) {
    int16_t* samples = (int16_t*)txBuffer;
    for (int i = 0; i < bytesRead / 2; i++) {
        samples[i] = (int16_t)((float)samples[i] * volume);
    }
    size_t bytesWritten = 0;
    i2s_write(I2S_NUM_1, txBuffer, bytesRead, &bytesWritten, portMAX_DELAY);
  }
  
  playFile.close();
  i2s_driver_uninstall(I2S_NUM_1);
}

// ─────────────────────────────────────────────
// Write Placeholder WAV Header to File
// ─────────────────────────────────────────────
void writeWAVHeader(File &file) {
  WAVHeader header;
  memcpy(header.riff, "RIFF", 4);
  header.fileSize = 0; // Placeholder
  memcpy(header.wave, "WAVE", 4);
  memcpy(header.fmt, "fmt ", 4);
  header.fmtSize = 16;
  header.audioFormat = 1;
  header.channels = CHANNELS;
  header.sampleRate = SAMPLE_RATE;
  header.bitsPerSample = BITS_PER_SAMPLE;
  header.byteRate = SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8);
  header.blockAlign = CHANNELS * (BITS_PER_SAMPLE / 8);
  memcpy(header.data, "data", 4);
  header.dataSize = 0; // Placeholder

  file.write((uint8_t*)&header, sizeof(header));
}

// ─────────────────────────────────────────────
// Update WAV Header After Recording is Complete
// ─────────────────────────────────────────────
void updateWAVHeader(File &file, uint32_t dataSize) {
  uint32_t fileSize = dataSize + sizeof(WAVHeader) - 8;
  file.seek(4);
  file.write((uint8_t*)&fileSize, 4);
  file.seek(40);
  file.write((uint8_t*)&dataSize, 4);
}

/* Generated via Copilot Space
   Sources:
   • espressif/arduino-esp32 – Official I2S API and examples
   • randomnerdtutorials.com – ESP32 project guides and pinouts
   • adafruit.com – INMP441 & MAX98357A hookup guides and libraries
   • sparkfun.com – Sensor integration best practices
   • arduino.cc – Core API references (Serial, SPI)
*/