# ESP32 RFID Audio Recorder - User Guide

## Quick Start Guide

### Initial Setup
1. **Hardware Assembly**: Follow the [Hardware Setup Guide](HARDWARE_SETUP.md)
2. **Library Installation**: Install required libraries per [Library Requirements](LIBRARY_REQUIREMENTS.md)
3. **Upload Sketch**: Upload `ESP32_RFID_Audio_Recorder.ino` to your ESP32
4. **Insert SD Card**: Format a microSD card as FAT32 and insert it
5. **Open Serial Monitor**: Set baud rate to 115200

### First Boot
Upon successful upload and boot, you should see:
```
=== ESP32 RFID Audio Recorder ===
Initializing system...
✓ GPIO initialized
✓ SPI buses initialized
✓ I2S audio system initialized
✓ SD Card initialized - Size: 32000MB
✓ RFID initialized - Version: 0x92

=== SYSTEM STATUS ===
Free Heap: 298000 bytes (Start: 300000)
SD Card: Ready
RFID: Ready
Current State: 0
RFID Present: No
Button Pressed: No
====================

System ready! Waiting for RFID tags...
Commands: 1=record test, 2=list files, 3=status, test=button test, beep=audio test
```

## Basic Operation

### Recording Your First Audio Message

#### Method 1: New RFID Tag
1. **Present RFID Tag**: Place an RFID tag near the RC522 reader
2. **Single Beep**: System plays confirmation beep (1000Hz)
3. **Countdown Beeps**: Three ascending beeps (800Hz → 1000Hz → 1200Hz)
4. **Recording**: LED turns on, record for 15 seconds
5. **Completion**: Three completion beeps, LED turns off
6. **File Saved**: Audio saved as `/RFID_[UID].wav`

**Serial Output Example:**
```
RFID detected: A1B2C3D4
Playing beep: 1000Hz for 500ms
Recording started...
Recording stopped. Saved 240000 samples (480000 bytes)
```

#### Method 2: Manual Recording Test
1. **Serial Command**: Type `1` in Serial Monitor
2. **Press Enter**: System starts manual recording
3. **Record Audio**: Speak for up to 15 seconds
4. **Auto Stop**: Recording stops automatically
5. **File Saved**: Audio saved as `/test_recording.wav`

### Playing Back Audio

#### Automatic Playback
1. **Present RFID Tag**: Place tag with existing recording near reader
2. **Immediate Playback**: Audio plays automatically
3. **Remove Tag**: Playback stops when tag is removed

**Serial Output Example:**
```
RFID detected: A1B2C3D4
Playback started: /RFID_A1B2C3D4.wav
Playback stopped
RFID removed
```

### Deleting and Re-recording

#### Delete and Record New
1. **Hold Button**: Press and hold the button
2. **Present RFID**: Place RFID tag near reader while button pressed
3. **Delete Beep**: Low-frequency confirmation beep (600Hz)
4. **Countdown**: Standard 3-beep countdown
5. **New Recording**: Record new 15-second message
6. **File Replaced**: Old file deleted, new file saved

**Serial Output Example:**
```
Button pressed
RFID detected: A1B2C3D4
Deleted file: /RFID_A1B2C3D4.wav
Playing beep: 600Hz for 500ms
Recording started...
```

## Serial Commands

### Interactive Commands
Open Serial Monitor (115200 baud) and type commands:

#### `1` - Manual Recording Test
Tests recording functionality without RFID
```
> 1
Manual recording test...
Recording started...
Recording stopped. Saved 240000 samples (480000 bytes)
```

#### `2` - List SD Card Files
Shows all files on SD card with sizes
```
> 2
SD Card files:
  RFID_A1B2C3D4.wav (480044 bytes)
  RFID_12345678.wav (480044 bytes)
  test_recording.wav (480044 bytes)
```

#### `3` - System Status
Displays comprehensive system information
```
> 3
=== SYSTEM STATUS ===
Free Heap: 295000 bytes (Start: 300000)
SD Card: Ready
RFID: Ready
Current State: 0
RFID Present: No
Button Pressed: No
====================

Memory - Free: 295000, Used: 5000
```

#### `test` - Button Test
Tests button functionality
```
> test
Button test - Press button now...
Button test PASSED!
```

#### `beep` - Audio Test
Tests audio system with test beep
```
> beep
Audio test - Playing test beep...
Playing beep: 1000Hz for 1000ms
```

#### `v[0-100]` - Volume Control
Future enhancement for volume adjustment
```
> v50
Volume control not yet implemented
```

## Understanding System States

### State Indicators

#### State 0: IDLE
- **LED**: Off
- **Behavior**: Waiting for RFID or button input
- **Description**: Normal standby state

#### State 1: RECORDING
- **LED**: Blinking (500ms intervals)
- **Behavior**: Recording audio to SD card
- **Duration**: 15 seconds maximum

#### State 2: PLAYING
- **LED**: Off
- **Behavior**: Playing audio from SD card
- **Duration**: Until RFID removed or file ends

#### State 3: NEW_BEEP
- **LED**: Off
- **Behavior**: Playing confirmation beep for new RFID
- **Duration**: 500ms

#### State 4: DELETE_BEEP
- **LED**: Off
- **Behavior**: Playing confirmation beep for deletion
- **Duration**: 500ms

#### State 5: RECORDING_BEEPS
- **LED**: Off
- **Behavior**: Playing countdown beeps before recording
- **Duration**: ~2.5 seconds (3 beeps + pauses)

#### State 6: ERROR
- **LED**: Rapid blinking (200ms intervals)
- **Behavior**: Error condition, attempting recovery
- **Duration**: 5 seconds before recovery attempt

## Advanced Features

### File Management

#### File Naming Convention
- **Format**: `/RFID_[UID].wav`
- **UID**: 8-character hexadecimal string from RFID tag
- **Example**: Tag with UID "A1B2C3D4" creates file `/RFID_A1B2C3D4.wav`

#### WAV File Format
- **Sample Rate**: 16kHz
- **Bit Depth**: 16-bit signed PCM
- **Channels**: Mono (1 channel)
- **File Size**: ~480KB for 15-second recording
- **Header**: Standard WAV header with proper chunk sizes

### Audio Quality Settings

#### Current Configuration
```cpp
Sample Rate: 16000 Hz
Bit Depth: 16 bits
Channels: 1 (Mono)
Recording Duration: 15 seconds
```

#### Customization Options
You can modify these parameters in the sketch:
```cpp
const uint32_t SAMPLE_RATE = 16000;        // Change sample rate
const uint16_t BITS_PER_SAMPLE = 16;       // Change bit depth
const uint32_t RECORDING_DURATION_MS = 15000; // Change duration
```

### RFID Tag Management

#### Supported RFID Types
- **Frequency**: 13.56MHz (ISO14443A)
- **Common Types**: MIFARE Classic, NTAG213/215/216
- **UID Length**: 4, 7, or 10 bytes (system uses first 4 bytes)

#### Detection Parameters
```cpp
RFID_CHECK_INTERVAL = 100ms     // How often to check for tags
RFID_GRACE_PERIOD = 3000ms      // Grace period for tag presence
RFID_REMOVAL_THRESHOLD = 4000ms // Time before considering tag removed
```

## Troubleshooting

### Common Issues and Solutions

#### No Audio Recording
**Symptoms**: Recording starts but no audio captured
**Solutions**:
1. Check INMP441 connections (especially power and I2S pins)
2. Verify microphone is not muted or damaged
3. Test with different microphone module
4. Check serial output for I2S errors

#### No Audio Playback
**Symptoms**: Playback starts but no sound output
**Solutions**:
1. Check speaker/amplifier connections
2. Verify speaker module power supply
3. Test with headphones if using amplifier module
4. Check I2S playback pin connections

#### RFID Not Detected
**Symptoms**: No response when presenting RFID tags
**Solutions**:
1. Verify RC522 power (3.3V, not 5V)
2. Check SPI connections (MOSI, MISO, SCK, SS)
3. Test with different RFID tags
4. Ensure tag is within 3cm of antenna

#### SD Card Errors
**Symptoms**: "SD Card initialization failed" or file errors
**Solutions**:
1. Reformat SD card as FAT32
2. Try different SD card (32GB or smaller recommended)
3. Check SPI connections to SD card module
4. Verify stable power supply

#### System Crashes or Resets
**Symptoms**: ESP32 resets unexpectedly
**Solutions**:
1. Check power supply stability (measure voltage under load)
2. Verify all ground connections
3. Monitor memory usage with command `3`
4. Add additional decoupling capacitors

#### Memory Issues
**Symptoms**: Heap allocation failures or poor performance
**Solutions**:
1. Monitor memory with command `3`
2. Reduce buffer sizes if needed
3. Enable PSRAM if available
4. Optimize recording duration

### Debug Information

#### Memory Monitoring
```cpp
// Check memory usage
Free Heap: 295000 bytes (Start: 300000)
Memory - Free: 295000, Used: 5000
```

#### Error State Recovery
The system automatically attempts recovery from error states:
- **Maximum Attempts**: 3 recovery attempts
- **Recovery Time**: 5 seconds between attempts
- **Manual Reset**: Power cycle if recovery fails

## Performance Optimization

### Power Consumption
- **Active Recording**: ~300-400mA
- **Idle State**: ~100-150mA
- **Playback**: ~200-300mA

### Storage Efficiency
- **15-second recording**: ~480KB
- **1GB SD card**: ~2000 recordings
- **32GB SD card**: ~65,000 recordings

### Response Times
- **RFID Detection**: < 100ms
- **Recording Start**: < 200ms
- **Playback Start**: < 500ms
- **Button Response**: < 50ms

## Best Practices

### Usage Guidelines
1. **Power Supply**: Use stable 5V/1A power source
2. **SD Card**: Use high-quality cards, format regularly
3. **RFID Tags**: Keep tags clean and undamaged
4. **Environment**: Avoid strong electromagnetic interference
5. **Maintenance**: Monitor memory usage regularly

### File Management
1. **Backup Important Recordings**: Copy files from SD card
2. **Monitor Storage**: Check available space periodically
3. **File Organization**: Use descriptive RFID tag labels
4. **Error Recovery**: Keep backup of critical recordings

### System Monitoring
1. **Serial Output**: Monitor for error messages
2. **Memory Usage**: Check with command `3` regularly
3. **File System**: List files with command `2`
4. **Audio Quality**: Test playback periodically