# Library Dependencies for ESP32 RFID Audio Recorder

## Required Arduino Libraries

### Core Libraries (Built-in with ESP32 Core)
- **WiFi.h** - ESP32 WiFi functionality (used for system includes)
- **SPI.h** - SPI communication protocol
- **driver/i2s.h** - ESP32 I2S audio driver
- **math.h** - Mathematical functions for audio processing

### External Libraries (Install via Library Manager)

#### 1. MFRC522 Library
- **Name**: MFRC522
- **Author**: GithubCommunity
- **Version**: 1.4.10 or later
- **Purpose**: RC522 RFID module communication
- **Installation**: Arduino IDE → Tools → Manage Libraries → Search "MFRC522"

#### 2. SD Library
- **Name**: SD
- **Author**: Arduino
- **Version**: Built-in with ESP32 core
- **Purpose**: SD card file system operations
- **Installation**: Automatically included with ESP32 board support

## Arduino IDE Setup

### Board Manager Configuration
1. Open Arduino IDE
2. Go to File → Preferences
3. Add ESP32 board manager URL:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to Tools → Board → Boards Manager
5. Search "ESP32" and install "ESP32 by Espressif Systems"

### Board Selection
- **Board**: ESP32 Dev Module (or your specific ESP32 variant)
- **Upload Speed**: 921600
- **CPU Frequency**: 240MHz (WiFi/BT)
- **Flash Frequency**: 80MHz
- **Flash Mode**: QIO
- **Flash Size**: 4MB (32Mb)
- **Partition Scheme**: Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)
- **Core Debug Level**: None
- **PSRAM**: Disabled

## Library Installation Instructions

### Method 1: Arduino Library Manager (Recommended)
1. Open Arduino IDE
2. Go to Tools → Manage Libraries
3. Search for "MFRC522"
4. Install "MFRC522" by GithubCommunity
5. Wait for installation to complete

### Method 2: Manual Installation
1. Download MFRC522 library from GitHub:
   ```
   https://github.com/miguelbalboa/rfid
   ```
2. Extract to Arduino libraries folder:
   - Windows: `Documents\Arduino\libraries\`
   - macOS: `~/Documents/Arduino/libraries/`
   - Linux: `~/Arduino/libraries/`
3. Restart Arduino IDE

## Compilation Requirements

### Compiler Flags
The sketch uses standard ESP32 Arduino framework features and should compile without additional flags.

### Memory Requirements
- **Flash Memory**: ~500KB for program + libraries
- **RAM Usage**: ~150KB during operation
- **PSRAM**: Not required (but can be enabled for additional headroom)

### ESP32 Core Version
- **Minimum**: 2.0.0
- **Recommended**: 2.0.11 or later
- **Maximum Tested**: 2.0.14

## Platform Compatibility

### Supported ESP32 Variants
- ESP32 (original)
- ESP32-S2
- ESP32-S3
- ESP32-C3

### Tested Development Boards
- ESP32 DevKit V1
- ESP32 WROOM-32
- ESP32 NodeMCU
- ESP32-S3 DevKit

## Version Information

### Library Versions (Tested)
```
ESP32 Arduino Core: 2.0.11
MFRC522: 1.4.10
SD: Built-in
SPI: Built-in
WiFi: Built-in
```

### Compilation Information
```cpp
// To check versions in your sketch:
Serial.println("ESP32 Arduino Core: " + String(ESP_ARDUINO_VERSION_MAJOR) + "." + 
               String(ESP_ARDUINO_VERSION_MINOR) + "." + 
               String(ESP_ARDUINO_VERSION_PATCH));
```

## Troubleshooting Library Issues

### Common Compilation Errors

#### Error: "MFRC522.h: No such file or directory"
**Solution**: Install MFRC522 library via Library Manager

#### Error: "driver/i2s.h: No such file or directory"
**Solution**: Update ESP32 core to version 2.0.0 or later

#### Error: "Multiple libraries found for SD.h"
**Solution**: Ensure using ESP32 SD library, not generic Arduino SD

#### Error: "WiFi.h conflicts"
**Solution**: Use ESP32 WiFi library, ensure proper ESP32 core installation

### Memory Compilation Warnings
If you receive warnings about memory usage:
1. Reduce I2S_BUFFER_SIZE if needed
2. Enable PSRAM if available
3. Optimize code structure
4. Use PROGMEM for constant data

### Performance Optimization
```cpp
// Add these for better performance
#pragma GCC optimize ("O2")
#define CONFIG_FREERTOS_ENABLE_BACKWARDS_COMPATIBILITY 1
```

## Development Environment Setup

### Recommended Tools
- **Arduino IDE**: 1.8.19 or Arduino IDE 2.x
- **ESP32 Exception Decoder**: For debugging crashes
- **ESP32 Filesystem Uploader**: For SPIFFS management (if needed)

### Alternative Platforms
- **PlatformIO**: Full compatibility with provided library configuration
- **ESP-IDF**: Can be ported with minor modifications
- **VSCode + PlatformIO**: Recommended for advanced development

## Testing Library Installation

### Verification Sketch
```cpp
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>
#include <driver/i2s.h>
#include <math.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Library Test");
  Serial.println("All libraries loaded successfully!");
}

void loop() {
  // Empty loop
}
```

### Expected Output
```
Library Test
All libraries loaded successfully!
```

## Library Documentation Links

### Official Documentation
- **ESP32 Arduino Core**: https://docs.espressif.com/projects/arduino-esp32/
- **MFRC522 Library**: https://github.com/miguelbalboa/rfid
- **ESP32 I2S Guide**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html

### Additional Resources
- **ESP32 SPI Documentation**: Hardware reference manual
- **SD Library Examples**: Built-in Arduino examples
- **I2S Audio Examples**: ESP32 community examples

## Support and Updates

### Staying Updated
1. Regularly update ESP32 core via Board Manager
2. Check for MFRC522 library updates
3. Monitor ESP32 Arduino framework releases
4. Subscribe to ESP32 community forums

### Getting Help
- Arduino IDE Help → Built-in documentation
- ESP32 Arduino GitHub issues
- Arduino community forums
- ESP32 official documentation