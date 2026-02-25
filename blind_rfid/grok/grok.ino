#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <driver/i2s.h>
#include <MFRC522.h>

// Hardware Configuration
const uint8_t SD_CS_PIN    = 5;
const uint8_t SD_MOSI_PIN  = 23;
const uint8_t SD_MISO_PIN  = 19;
const uint8_t SD_SCK_PIN   = 18;

const uint8_t I2S0_WS_PIN      = 25;
const uint8_t I2S0_SCK_PIN     = 26;
const uint8_t I2S0_RX_DATA_PIN = 27;

const uint8_t I2S1_WS_PIN      = 32;
const uint8_t I2S1_SCK_PIN     = 14;
const uint8_t I2S1_TX_DATA_PIN = 33;

const uint8_t RFID_RST_PIN  = 22;
const uint8_t RFID_SS_PIN   = 4;
const uint8_t RFID_MOSI_PIN = 13;
const uint8_t RFID_MISO_PIN = 12;
const uint8_t RFID_SCK_PIN  = 16;

const uint8_t BUTTON_PIN    = 15;

const uint32_t SAMPLE_RATE     = 16000;
const uint8_t  BITS_PER_SAMPLE = 16;
const uint8_t  CHANNELS        = 1;
const uint8_t  RECORD_SECONDS  = 15;

// WAV File Header Structure
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

// Global Variables
File audioFile;
char wavFilename[32] = "/norec.wav";
float volume = 1.0;
SPIClass rfidSPI(VSPI);
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
unsigned long lastSerialCheck = 0;
const long serialCheckInterval = 100;

// Function Declarations
bool initSDCard();
bool initI2SRecording();
bool initI2SPlayback();
void recordAudio(const char* filename);
void listSDFiles();
void playbackAudio(const char* filename);
void writeWAVHeader(File &file);
void updateWAVHeader(File &file, uint32_t dataSize);
void processSerialCommands();
void setVolume(int volPercent);
void handleRFID();

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(1); }
  
  Serial.println(F("\nESP32 RFID Audio Recorder/Player"));
  Serial.println(F("────────────────────────────────"));
  Serial.println(F("Commands:"));
  Serial.println(F("  1 – Record 15-sec WAV file (serial)"));
  Serial.println(F("  2 – List SD card files"));
  Serial.println(F("  3 – Playback last recorded file (serial)"));
  Serial.println(F("  v<num> – Set volume (0-100), e.g., v70"));
  Serial.println(F("RFID: Bring tag near RC522"));
  Serial.println(F("  - Press button (GPIO 15) to record"));
  Serial.println(F("  - No button to playback if exists"));
  Serial.println(F("────────────────────────────────"));

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!initSDCard()) {
    Serial.println(F("FATAL: SD Card initialization FAILED. Halting."));
    while (true) { delay(1000); }
  }
  
  if (!initI2SRecording()) {
    Serial.println(F("FATAL: I2S Recording initialization FAILED. Halting."));
    while (true) { delay(1000); }
  }

  Serial.print(F("Initializing RC522... "));
  rfidSPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max); // Optional: Maximize antenna gain
  if (rfid.PCD_PerformSelfTest()) {
    Serial.println(F("OK."));
  } else {
    Serial.println(F("FAILED. Check wiring."));
  }
  rfidSPI.end();

  setVolume(70);
}

void loop() {
  if (millis() - lastSerialCheck >= serialCheckInterval) {
    processSerialCommands();
    handleRFID();
    lastSerialCheck = millis();
  }
}

void processSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equals("1")) {
      Serial.println(F("COMMAND: Record Audio"));
      snprintf(wavFilename, sizeof(wavFilename), "/REC%08lX.wav", (uint32_t)millis());
      recordAudio(wavFilename);
      Serial.println(F("ACTION: Recording complete."));
    } else if (command.equals("2")) {
      Serial.println(F("COMMAND: List Files"));
      listSDFiles();
      Serial.println(F("ACTION: File list complete."));
    } else if (command.equals("3")) {
      Serial.println(F("COMMAND: Playback Audio"));
      playbackAudio(wavFilename);
      Serial.println(F("ACTION: Playback complete."));
    } else if (command.startsWith("v")) {
      Serial.println(F("COMMAND: Set Volume"));
      long volPercent = command.substring(1).toInt();
      if (volPercent >= 0 && volPercent <= 100) {
        setVolume(volPercent);
      } else {
        Serial.println(F("ERROR: Invalid volume. Use 0-100."));
      }
    } else if (command.length() > 0) {
      Serial.print(F("ERROR: Unknown command '"));
      Serial.print(command);
      Serial.println(F("'"));
    }
  }
}

void handleRFID() {
  rfidSPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    rfidSPI.end();
    return;
  }

  String uidStr = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidStr += "0";
    uidStr += String(rfid.uid.uidByte[i], HEX);
  }
  uidStr.toUpperCase();
  snprintf(wavFilename, sizeof(wavFilename), "/%s.wav", uidStr.c_str());

  Serial.print(F("INFO: RFID Tag Detected, UID: "));
  Serial.println(uidStr);

  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
  if (buttonPressed) {
    Serial.println(F("INFO: Button pressed - Recording..."));
    rfidSPI.end();
    recordAudio(wavFilename);
  } else if (SD.exists(wavFilename)) {
    Serial.println(F("INFO: Playing back existing recording..."));
    rfidSPI.end();
    playbackAudio(wavFilename);
  } else {
    Serial.println(F("INFO: No recording exists for this tag."));
    rfidSPI.end();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void setVolume(int volPercent) {
  volume = constrain(volPercent, 0, 100) / 100.0f;
  Serial.print(F("INFO: Volume set to "));
  Serial.print(volPercent);
  Serial.println(F("%."));
}

bool initSDCard() {
  Serial.print(F("Initializing SD card... "));
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SPI)) {
    Serial.println(F("FAILED."));
    return false;
  }
  Serial.println(F("OK."));
  return true;
}

bool initI2SRecording() {
  Serial.print(F("Initializing I2S for Recording... "));
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

bool initI2SPlayback() {
  Serial.print(F("Initializing I2S for Playback... "));
  i2s_driver_uninstall(I2S_NUM_1);
  
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

void recordAudio(const char* filename) {
  audioFile = SD.open(filename, FILE_WRITE);
  if (!audioFile) {
    Serial.println(F("ERROR: Could not create file."));
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
  Serial.print(filename);
  Serial.print(F(" ("));
  Serial.print(bytesRecorded / 1024);
  Serial.println(F(" KB)"));
}

void listSDFiles() {
  File root = SD.open("/");
  if (!root) {
    Serial.println(F("ERROR: Failed to open root directory."));
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

void playbackAudio(const char* filename) {
  File playFile = SD.open(filename, FILE_READ);
  if (!playFile) {
    Serial.print(F("ERROR: Playback file not found: "));
    Serial.println(filename);
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

void writeWAVHeader(File &file) {
  WAVHeader header;
  memcpy(header.riff, "RIFF", 4);
  header.fileSize = 0;
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
  header.dataSize = 0;

  file.write((uint8_t*)&header, sizeof(header));
}

void updateWAVHeader(File &file, uint32_t dataSize) {
  uint32_t fileSize = dataSize + sizeof(WAVHeader) - 8;
  file.seek(4);
  file.write((uint8_t*)&fileSize, 4);
  file.seek(40);
  file.write((uint8_t*)&dataSize, 4);
}