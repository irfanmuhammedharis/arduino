# User Guide - ESP32 RFID Audio Recorder

## 🚀 Quick Start Guide

### First Time Setup
1. **Prepare Hardware**: Assemble according to [HARDWARE_SETUP.md](HARDWARE_SETUP.md)
2. **Install Libraries**: Follow [LIBRARY_REQUIREMENTS.md](LIBRARY_REQUIREMENTS.md)
3. **Upload Code**: Load `ESP32_RFID_Audio_Recorder.ino` to your ESP32
4. **Format SD Card**: Ensure microSD card is formatted as FAT32
5. **Test System**: Connect serial monitor at 115200 baud

### Initial Power-On
```
1. Power on the ESP32
2. Watch for startup messages in serial monitor
3. LED should blink slowly (system ready)
4. Type 'help' in serial monitor for commands
```

## 🎯 Basic Operation

### Recording Audio
```
Step 1: Place RFID tag on reader
        → LED turns solid (tag detected)

Step 2: Press and hold button
        → LED blinks fast (recording active)
        → Speak into microphone

Step 3: Release button
        → LED returns to solid (recording saved)
        → File saved as "audio_[UID].wav"
```

### Playing Back Audio
```
Step 1: Place same RFID tag on reader
        → LED turns solid

Step 2: Long press button (hold >1 second)
        → LED blinks slowly (playback active)
        → Audio plays through speaker/headphones

Step 3: Playback ends automatically
        → LED returns to solid
```

### Returning to Idle
```
Remove RFID tag from reader
→ LED returns to slow blinking (idle state)
→ System ready for next tag
```

## 💡 LED Status Reference

| LED Pattern | System State | Description |
|-------------|--------------|-------------|
| **Slow Blink** | Idle/Ready | System ready, waiting for RFID tag |
| **Solid On** | Tag Detected | RFID tag detected, waiting for button |
| **Fast Blink** | Recording/Error | Audio recording active OR system error |
| **Off** | Startup/Critical Error | System initializing or critical failure |

### LED Timing Details
- **Slow Blink**: 1 second on, 1 second off
- **Fast Blink**: 200ms on, 200ms off  
- **Solid**: Continuously on
- **Off**: Continuously off

## 🎛 Serial Command Interface

Connect to serial monitor (115200 baud) for advanced control and debugging.

### System Commands

#### `status` or `s`
Shows complete system information:
```
====== SYSTEM STATUS ======
Uptime: 1234 seconds
State: Waiting for Button
RFID Tag Present: Yes
Current UID: A1B2C3D4
Button State: Released
Recording: No
Playback: No
Free Heap: 256000 bytes
SD Card Total: 4000 KB
SD Card Free: 3500 KB
===========================
```

#### `help` or `h` or `?`
Displays complete command reference and usage instructions.

#### `reset` or `restart`
Restarts the entire system (equivalent to power cycle).

#### `test` or `t`
Tests all hardware components:
```
====== HARDWARE TEST ======
Testing LED... OK
Testing Button (press now)... OK
Testing RFID... OK
Testing SD Card... OK
Test Result: ALL PASSED
===========================
```

### RFID Commands

#### `rfid` or `r`
Comprehensive RFID diagnostics:
```
====== RFID DIAGNOSTICS ======
RFID Reader Status:
  Model: RC522
  Current State: Tag Present
  Current UID: A1B2C3D4
  Detection Count: 15
  Last Detection: 100ms ago

RFID Configuration:
  Removal Delay: 1000ms
  Retry Attempts: 3
  Scan Interval: 50ms

Hardware Test:
  Self-test: PASS
  Antenna Status: Active
  Firmware Version: 0x92
==============================
```

### Audio Commands

#### `files` or `f`
Lists all audio files on SD card:
```
====== AUDIO FILES ======
1. audio_A1B2C3D4.wav (480000 bytes)
2. audio_12345678.wav (720000 bytes)
3. audio_ABCDEF12_1.wav (360000 bytes)
Total: 3 audio files
=========================
```

#### `delete <filename>`
Deletes specific audio file:
```
Example: delete audio_A1B2C3D4.wav
Result: File deleted successfully
```

#### `record` or `rec`
Test recording function (requires RFID tag):
```
Starting test recording...
Recording for 3 seconds...
Recording complete: audio_test.wav
```

#### `play` or `p`
Test playback function (requires RFID tag with existing audio):
```
Starting playback: audio_A1B2C3D4.wav
Estimated duration: 8 seconds
Playback started...
```

### Debug Commands

#### `memory` or `mem`
Shows detailed memory information:
```
Memory Status:
  Free Heap: 245760 bytes
  Initial Heap: 290000 bytes
  Used: 44240 bytes
  Largest Free Block: 180000 bytes
```

#### `debug <0-4>`
Sets debug verbosity level:
- **0**: No debug output
- **1**: Errors only
- **2**: Warnings and errors
- **3**: General information (default)
- **4**: Verbose debugging

#### `beep` or `b`
Tests audio feedback system (LED flashes).

#### `clear` or `cls`
Clears the serial terminal screen.

## 🎵 Audio File Management

### File Naming Convention
- **Format**: `audio_[RFID_UID].wav`
- **Example**: `audio_A1B2C3D4.wav`
- **Duplicates**: Numbered automatically (`audio_A1B2C3D4_1.wav`)

### File Specifications
```
Format:        WAV (PCM)
Sample Rate:   16 kHz
Bit Depth:     16-bit
Channels:      Mono
Max Duration:  15 seconds per recording
File Size:     ~480 KB per 15-second recording
```

### Storage Management
- **Capacity**: ~8,500 recordings on 4GB SD card
- **Organization**: All files in root directory
- **Backup**: Copy files to computer via SD card reader
- **Cleanup**: Use `delete` command or format SD card

### File Operations
```bash
# View files
files

# Delete specific file
delete audio_A1B2C3D4.wav

# Delete numbered variant
delete audio_12345678_2.wav

# Check storage space
status
```

## 🔧 Configuration & Customization

### Hardware Configuration
Edit `config.h` to adapt to your hardware:

```cpp
// Pin assignments
#define RFID_SS_PIN     5      // RFID chip select
#define BUTTON_PIN      21     // Button input pin
#define STATUS_LED_PIN  2      // Status LED pin

// Timing parameters
#define BUTTON_DEBOUNCE_DELAY   50    // Button debounce (ms)
#define RFID_REMOVAL_DELAY      1000  // RFID removal delay (ms)
#define RECORD_TIME             15    // Max recording time (seconds)

// Audio settings
#define SAMPLE_RATE     16000   // Audio sample rate (Hz)
#define DMA_BUF_COUNT   8       // Number of DMA buffers
```

### Performance Tuning
```cpp
// For faster response
#define RFID_SCAN_INTERVAL      25    // Faster RFID scanning
#define BUTTON_DEBOUNCE_DELAY   25    // Faster button response

// For better audio quality
#define SAMPLE_RATE     22050   // Higher sample rate
#define DMA_BUF_COUNT   16      // More buffers for stability

// For longer recordings
#define RECORD_TIME     30      // 30-second recordings
```

### Debug Configuration
```cpp
// Enable detailed logging
#define DEBUG_LEVEL     DEBUG_LEVEL_VERBOSE

// Enable additional features
#define ENABLE_DIAGNOSTICS      true
#define ENABLE_SERIAL_COMMANDS  true
```

## 🛠 Troubleshooting Guide

### Common Issues & Solutions

#### Issue: LED Not Working
**Symptoms**: No LED activity, system appears unresponsive
```
Diagnosis:
1. Check LED wiring (GPIO 2 → Resistor → LED → GND)
2. Test with multimeter: GPIO 2 should toggle 0V/3.3V
3. Verify LED polarity (longer leg = positive)

Solutions:
- Rewire LED connections
- Try different LED
- Check resistor value (220Ω recommended)
```

#### Issue: Button Not Responding
**Symptoms**: Button presses ignored, no recording starts
```
Diagnosis:
1. Use 'test' command to check button
2. Monitor serial for button press messages
3. Check physical button wiring

Solutions:
- Verify button connected: GPIO 21 → Button → GND
- Check button type (normally open required)
- Test with different button
- Check for loose connections
```

#### Issue: RFID Not Detecting Tags
**Symptoms**: No response when placing RFID tags
```
Diagnosis:
1. Run 'rfid' command for diagnostics
2. Check self-test result
3. Verify tag compatibility

Solutions:
- Check SPI wiring (SCK, MOSI, MISO, SS, RST)
- Verify 3.3V power to RC522 (NOT 5V!)
- Try different RFID tags
- Check antenna connections
- Reduce distance between tag and reader
```

#### Issue: SD Card Problems
**Symptoms**: "SD Card initialization failed" errors
```
Diagnosis:
1. Check 'status' command for SD card info
2. Verify card format (must be FAT32)
3. Test with different SD card

Solutions:
- Format SD card as FAT32
- Check CS pin connection (GPIO 15)
- Verify shared SPI connections
- Try lower capacity SD card (<32GB)
- Check power supply adequacy
```

#### Issue: Audio Recording Problems
**Symptoms**: No audio files created or corrupt recordings
```
Diagnosis:
1. Check I2S wiring (GPIO 25, 26, 33)
2. Verify microphone power (3.3V)
3. Test with 'record' command

Solutions:
- Check I2S pin connections
- Verify microphone orientation
- Ensure L/R pin connected to GND
- Check for electrical noise
- Test with different microphone
```

#### Issue: System Crashes or Resets
**Symptoms**: ESP32 reboots unexpectedly
```
Diagnosis:
1. Monitor serial output for crash logs
2. Check 'memory' command for heap issues
3. Verify power supply stability

Solutions:
- Use adequate power supply (500mA+)
- Check for memory leaks
- Reduce audio buffer sizes
- Add power supply filtering
- Check for loose connections
```

### Advanced Diagnostics

#### Serial Monitor Debug Messages
```
[INFO] System ready. Place RFID tag to start.
[DEBUG] Button pressed
[VERBOSE] RFID Tag removal detected - starting verification...
[ERROR] SD Card initialization failed
[WARN] Possible memory leak detected
```

#### Performance Monitoring
```
# Check system health
status

# Monitor memory usage
memory

# Test all components
test

# Detailed RFID diagnostics
rfid
```

#### Hardware Signal Testing
Use oscilloscope or logic analyzer to verify:
- **SPI Clock**: Clean 4MHz square wave
- **I2S Signals**: Proper audio clocking
- **Power Rails**: Stable 3.3V with minimal ripple
- **Button Signal**: Clean transitions with debouncing

## 📊 Performance Optimization

### Audio Quality Enhancement
```cpp
// Higher sample rate for better quality
#define SAMPLE_RATE     22050

// More DMA buffers for stability
#define DMA_BUF_COUNT   16
#define DMA_BUF_LEN     1024

// Larger audio buffer
#define AUDIO_BUFFER_SIZE  2048
```

### System Response Tuning
```cpp
// Faster button response
#define BUTTON_DEBOUNCE_DELAY   25

// Quicker RFID scanning
#define RFID_SCAN_INTERVAL      25

// Reduced removal delay for faster operation
#define RFID_REMOVAL_DELAY      500
```

### Memory Optimization
```cpp
// Smaller buffers for memory-constrained systems
#define DMA_BUF_COUNT   4
#define DMA_BUF_LEN     256
#define AUDIO_BUFFER_SIZE  512
```

## 🔒 Security & Privacy

### Data Security
- **Local Storage**: All audio files stored locally on SD card
- **No Network**: No WiFi or Bluetooth transmission by default
- **Physical Access**: Secure SD card access if needed
- **File Encryption**: Add encryption if handling sensitive data

### Privacy Considerations
- **Recording Indication**: LED clearly shows when recording
- **User Control**: Recording only when button pressed
- **File Management**: Easy deletion via serial commands
- **Access Control**: Physical device access required

## 📈 Advanced Usage

### Batch Operations
```bash
# List all files and sizes
files

# Delete old recordings (manually specify)
delete audio_OLD_TAG.wav
delete audio_TEMP_123.wav

# Check available space
status
```

### Automation Scripts
Create simple scripts for common tasks:

```python
# Python script for SD card management
import serial
import time

ser = serial.Serial('/dev/ttyUSB0', 115200)
time.sleep(2)

# Get file list
ser.write(b'files\n')
time.sleep(1)
response = ser.read_all()
print(response.decode())

# Clean up old files
old_files = ['audio_12345678.wav', 'audio_ABCDEF.wav']
for file in old_files:
    command = f'delete {file}\n'
    ser.write(command.encode())
    time.sleep(0.5)

ser.close()
```

### Integration with Other Systems
- **Data Logging**: Parse serial output for system monitoring
- **File Transfer**: Automated SD card file management
- **Remote Control**: Serial command automation
- **Status Monitoring**: Health check integration

This user guide provides comprehensive information for operating the ESP32 RFID Audio Recorder system effectively. For hardware setup details, refer to [HARDWARE_SETUP.md](HARDWARE_SETUP.md), and for library installation, see [LIBRARY_REQUIREMENTS.md](LIBRARY_REQUIREMENTS.md).