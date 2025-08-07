# Library Requirements - ESP32 RFID Audio Recorder

## 📚 Required Libraries

### 1. ESP32 Core Library
**Installation via Board Manager:**
```
File → Preferences → Additional Board Manager URLs:
https://dl.espressif.com/dl/package_esp32_index.json

Tools → Board → Board Manager → Search "ESP32" → Install
```
- **Version**: 2.0.0 or higher
- **Description**: Core ESP32 support with I2S, SPI, and WiFi drivers
- **Publisher**: Espressif Systems

### 2. MFRC522 Library
**Installation via Library Manager:**
```
Sketch → Include Library → Manage Libraries → Search "MFRC522"
```
- **Library Name**: MFRC522
- **Version**: 1.4.10 or higher
- **Author**: GithubCommunity
- **Description**: Arduino library for MFRC522 RFID reader
- **GitHub**: https://github.com/miguelbalboa/rfid

### 3. Built-in ESP32 Libraries (Pre-installed)
The following libraries are included with the ESP32 core:

#### SPI Library
- **Include**: `#include <SPI.h>`
- **Description**: Serial Peripheral Interface communication
- **Used for**: RFID reader and SD card communication

#### SD Library  
- **Include**: `#include <SD.h>`
- **Description**: SD card file system operations
- **Used for**: Audio file storage and management

#### FS Library
- **Include**: `#include <FS.h>`
- **Description**: File system abstraction layer
- **Used for**: File operations and SD card interface

#### I2S Driver
- **Include**: `#include <driver/i2s.h>`
- **Description**: Inter-IC Sound interface driver
- **Used for**: Audio recording and playback

## 🔧 Arduino IDE Configuration

### Board Settings
```
Board: "ESP32 Dev Module"
Upload Speed: "921600"
CPU Frequency: "240MHz (WiFi/BT)"
Flash Frequency: "80MHz"
Flash Mode: "QIO"
Flash Size: "4MB (32Mb)"
Partition Scheme: "Default 4MB with spiffs"
Core Debug Level: "None"
PSRAM: "Disabled"
```

### Compilation Flags
Add these to your platform.local.txt if needed:
```
compiler.cpp.extra_flags=-DCONFIG_ARDUHAL_LOG_COLORS=1
```

## 📦 Library Installation Guide

### Method 1: Arduino IDE Library Manager (Recommended)
1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search for "MFRC522"
4. Select the library by GithubCommunity
5. Click **Install**
6. Wait for installation to complete

### Method 2: Manual Installation
1. Download MFRC522 library from GitHub
2. Extract to your Arduino libraries folder:
   - **Windows**: `Documents/Arduino/libraries/`
   - **Mac**: `Documents/Arduino/libraries/`
   - **Linux**: `~/Arduino/libraries/`
3. Restart Arduino IDE

### Method 3: Git Clone (Advanced)
```bash
cd ~/Arduino/libraries/
git clone https://github.com/miguelbalboa/rfid.git MFRC522
```

## ✅ Verification Steps

### 1. Check Library Installation
```cpp
// Test sketch to verify libraries
#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>
#include <FS.h>
#include <driver/i2s.h>

void setup() {
  Serial.begin(115200);
  Serial.println("All libraries loaded successfully!");
}

void loop() {
  // Empty
}
```

### 2. Compilation Test
1. Open the test sketch above
2. Select your ESP32 board
3. Click **Verify** (checkmark icon)
4. Should compile without errors

### 3. Library Versions
Check installed library versions:
```
Tools → Manage Libraries → Filter: Installed
```
Verify MFRC522 is version 1.4.10 or higher.

## 🔍 Troubleshooting

### Common Installation Issues

#### "Library not found" Error
```
Error: fatal error: MFRC522.h: No such file or directory
```
**Solution:**
- Restart Arduino IDE after library installation
- Check library is in correct folder
- Verify library name is exactly "MFRC522"

#### ESP32 Board Not Found
```
Error: Board package not installed
```
**Solution:**
- Add ESP32 board URL to preferences
- Install ESP32 board package via Board Manager
- Select correct board: "ESP32 Dev Module"

#### I2S Driver Issues
```
Error: 'i2s_config_t' was not declared in this scope
```
**Solution:**
- Update ESP32 core to version 2.0.0+
- Include correct header: `#include <driver/i2s.h>`
- Check board is set to ESP32 (not Arduino Uno)

#### SD Library Conflicts
```
Error: Multiple libraries found for "SD.h"
```
**Solution:**
- Use ESP32's built-in SD library
- Remove other SD libraries from libraries folder
- Clear Arduino IDE cache

### Library Compatibility

#### MFRC522 Library Versions
- **1.4.10**: Latest stable, recommended
- **1.4.9**: Compatible, minor bug fixes in 1.4.10
- **1.4.8**: Minimum version, some features missing
- **<1.4.8**: Not recommended, compatibility issues

#### ESP32 Core Versions
- **2.0.5**: Latest stable, best performance
- **2.0.4**: Compatible, minor improvements in 2.0.5
- **2.0.0-2.0.3**: Compatible, basic functionality
- **1.x.x**: Not compatible, major API changes

## 📋 Development Environment Setup

### Recommended IDE Settings
```
Editor Font Size: 12pt
Tab Size: 2 spaces
Auto-format: Enabled
Line Numbers: Enabled
Syntax Highlighting: Enabled
```

### Serial Monitor Configuration
```
Baud Rate: 115200
Line Ending: "Newline"
Timestamp: Enabled (optional)
Autoscroll: Enabled
```

### Additional Tools (Optional)

#### PlatformIO (Alternative IDE)
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    miguelbalboa/MFRC522@^1.4.10
monitor_speed = 115200
```

#### ESP32 Sketch Data Upload Tool
For uploading files to SPIFFS if needed:
1. Download ESP32SketchDataUpload
2. Install in Arduino IDE tools folder
3. Use Tools → ESP32 Sketch Data Upload

## 📊 Memory Requirements

### Flash Memory Usage
```
MFRC522 Library:     ~15KB
ESP32 Core:          ~200KB
Application Code:    ~180KB
Total Flash Used:    ~395KB (10% of 4MB)
Available for Data:  ~3.6MB
```

### RAM Usage
```
MFRC522 Variables:   ~2KB
Audio Buffers:       ~8KB (configurable)
System Variables:    ~40KB
Free RAM:           ~270KB
```

## 🔄 Update Procedures

### Updating Libraries
1. **Check for Updates**:
   ```
   Tools → Manage Libraries → Filter: Updatable
   ```

2. **Update MFRC522**:
   - Click on MFRC522 library
   - Select latest version
   - Click Update

3. **Update ESP32 Core**:
   ```
   Tools → Board → Boards Manager → Search "ESP32" → Update
   ```

### Backup Before Updates
```bash
# Backup libraries folder
cp -r ~/Arduino/libraries/ ~/Arduino/libraries_backup/

# Backup board definitions
cp -r ~/.arduino15/packages/ ~/.arduino15/packages_backup/
```

## 🧪 Testing Library Installation

### Complete Test Sketch
```cpp
/*
 * Library Installation Test
 * Tests all required libraries and components
 */

#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>
#include <FS.h>
#include <driver/i2s.h>

#define SS_PIN 5
#define RST_PIN 22
#define SD_CS 15

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== Library Installation Test ===");
  
  // Test SPI
  Serial.print("Testing SPI... ");
  SPI.begin();
  Serial.println("OK");
  
  // Test RFID
  Serial.print("Testing RFID... ");
  rfid.PCD_Init();
  if (rfid.PCD_PerformSelfTest()) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED - Check wiring");
  }
  
  // Test SD Card
  Serial.print("Testing SD Card... ");
  if (SD.begin(SD_CS)) {
    Serial.println("OK");
    Serial.println("  Size: " + String(SD.totalBytes() / 1024) + " KB");
  } else {
    Serial.println("FAILED - Check SD card");
  }
  
  // Test I2S
  Serial.print("Testing I2S... ");
  i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024
  };
  
  if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) == ESP_OK) {
    Serial.println("OK");
    i2s_driver_uninstall(I2S_NUM_0);
  } else {
    Serial.println("FAILED");
  }
  
  Serial.println("=== Test Complete ===");
  Serial.println("All libraries installed correctly!");
}

void loop() {
  // Test complete
}
```

Save this as a test sketch and upload to verify all libraries are working correctly.

## 📞 Support Resources

### Official Documentation
- **ESP32**: https://docs.espressif.com/projects/esp-idf/
- **MFRC522**: https://github.com/miguelbalboa/rfid
- **Arduino ESP32**: https://github.com/espressif/arduino-esp32

### Community Support
- **Arduino Forum**: https://forum.arduino.cc/
- **ESP32 Reddit**: https://www.reddit.com/r/esp32/
- **GitHub Issues**: https://github.com/miguelbalboa/rfid/issues

### Troubleshooting Resources
- **ESP32 Troubleshooting**: https://github.com/espressif/arduino-esp32/issues
- **Library Compatibility**: https://www.arduinolibraries.info/