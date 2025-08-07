# ESP32 RFID Audio Recorder

An ESP32-based RFID-triggered audio recording system with robust button handling and stable RFID detection.

## Features

- **RFID-triggered recording**: Place an RFID tag to activate the system
- **Button-controlled recording**: Press and hold to record, release to stop
- **Long-press playback**: Long press to play back recorded audio for the current RFID tag
- **Stable RFID detection**: Improved timing to prevent false "tag removed" notifications
- **Robust button handling**: Proper debouncing and press detection
- **SD card storage**: Audio files stored as WAV format
- **Serial interface**: Debug commands and system status
- **LED status indication**: Visual feedback for system state

## Hardware Requirements

- ESP32 Development Board
- RC522 RFID Reader Module
- INMP441 I2S Microphone
- MicroSD Card Module
- Push Button
- LED with 220Ω resistor
- Breadboard and jumper wires

## Pin Connections

### RC522 RFID Reader
| RC522 Pin | ESP32 Pin |
|-----------|-----------|
| SDA       | GPIO 5    |
| SCK       | GPIO 18   |
| MOSI      | GPIO 23   |
| MISO      | GPIO 19   |
| RST       | GPIO 22   |
| 3.3V      | 3.3V      |
| GND       | GND       |

### SD Card Module
| SD Pin | ESP32 Pin |
|--------|-----------|
| CS     | GPIO 15   |
| SCK    | GPIO 18   |
| MOSI   | GPIO 23   |
| MISO   | GPIO 19   |
| VCC    | 3.3V      |
| GND    | GND       |

### I2S Microphone (INMP441)
| Mic Pin | ESP32 Pin |
|---------|-----------|
| SCK     | GPIO 26   |
| WS      | GPIO 25   |
| SD      | GPIO 33   |
| VDD     | 3.3V      |
| GND     | GND       |

### Button and LED
| Component | ESP32 Pin |
|-----------|-----------|
| Button    | GPIO 4 (to GND) |
| LED       | GPIO 2 (through 220Ω resistor) |

## Installation

1. Install the Arduino IDE
2. Install ESP32 board support
3. Install required libraries:
   - MFRC522 (for RFID)
   - SD (usually included)
   - FS (usually included)

4. Connect the hardware according to the pin connections above
5. Upload the `ESP32_RFID_Audio_Recorder.ino` sketch to your ESP32

## Usage

### Basic Operation

1. **Power on**: The LED will blink slowly indicating the system is ready
2. **Place RFID tag**: The LED will turn solid, indicating tag detection
3. **Record audio**: Press and hold the button to record. LED blinks fast during recording
4. **Stop recording**: Release the button to stop recording
5. **Playback**: Long press the button (>1 second) to play back the recorded audio
6. **Remove tag**: Remove the RFID tag to return to idle state

### LED Status Indicators

- **Slow blink**: System ready, waiting for RFID tag
- **Solid on**: RFID tag detected, waiting for button press
- **Fast blink**: Recording in progress or error state
- **Off**: System error or not initialized

### Serial Commands

Connect to the serial monitor (115200 baud) to use debug commands:

- `status` - Show current system status
- `files` - List all audio files on SD card
- `delete <filename>` - Delete a specific audio file
- `reset` - Restart the system
- `help` - Show available commands

## Key Improvements

### Button Detection Fixes
- **Proper debouncing**: 50ms debounce delay prevents false triggers
- **Long press detection**: Distinguishes between short press (record) and long press (playback)
- **Real-time feedback**: Button state changes are immediately logged
- **Press duration tracking**: Accurate timing for button press events

### RFID Detection Fixes
- **Removal delay**: 1000ms delay before confirming tag removal
- **Retry mechanism**: Multiple attempts to verify tag presence
- **State verification**: Prevents premature "tag removed" notifications
- **Stable detection window**: 200ms window for consistent tag detection

### System Stability Improvements
- **Robust state machine**: Clear state transitions with proper error handling
- **SPI bus management**: Proper sharing between RFID and SD card
- **Error recovery**: Automatic recovery attempts from error states
- **Memory management**: Efficient buffer handling for audio data

## File Format

Audio files are saved as WAV format with the following specifications:
- **Sample rate**: 16 kHz
- **Bit depth**: 16-bit
- **Channels**: Mono
- **Format**: PCM
- **Filename**: `audio_<RFID_UID>.wav`

## Troubleshooting

### Common Issues

1. **SD card not detected**
   - Check wiring connections
   - Ensure SD card is formatted as FAT32
   - Verify power supply is adequate

2. **RFID not working**
   - Check SPI connections
   - Ensure RFID module has proper power
   - Try different RFID tags

3. **Button not responding**
   - Verify button wiring (pull-up enabled internally)
   - Check debounce timing in serial output
   - Ensure good connections

4. **Audio quality issues**
   - Check I2S microphone connections
   - Verify power supply stability
   - Adjust sample rate if needed

### Debug Information

Use the serial monitor to view:
- System state changes
- Button press events
- RFID detection/removal events
- File operations
- Error messages

## License

This project is open source. Feel free to modify and improve the code.