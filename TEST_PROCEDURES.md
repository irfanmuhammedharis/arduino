# Test Procedures for ESP32 RFID Audio Recorder

This document outlines test procedures to verify the fixes for button detection and RFID timing issues.

## Pre-Test Setup

1. Upload the code to ESP32
2. Connect serial monitor at 115200 baud
3. Ensure all hardware is properly connected
4. Have RFID tags ready for testing

## Test 1: Button Detection and Debouncing

### Objective
Verify that button presses are properly detected and debounced.

### Procedure
1. Power on the system
2. Place an RFID tag on the reader
3. Perform the following button tests:
   - **Quick press and release** (< 100ms)
   - **Normal press and release** (200-500ms)
   - **Long press** (> 1 second)
   - **Multiple rapid presses** (simulate bouncing)
   - **Press and hold for 3 seconds**

### Expected Results
- Each button press should be logged in serial output
- No false triggers from button bounce
- Clear distinction between short and long presses
- Recording starts on press, stops on release
- Playback starts on long press

### Serial Output Examples
```
Button pressed
Short button press detected
Recording started: /audio_A1B2C3D4.wav
Button pressed
Recording stopped. Samples recorded: 8000

Button pressed
Long button press detected (while held)
Playback started: /audio_A1B2C3D4.wav
```

## Test 2: RFID Tag Detection Stability

### Objective
Verify that RFID tags are not falsely reported as removed.

### Procedure
1. Power on the system
2. Place RFID tag on reader and hold steady
3. Observe serial output for 30 seconds
4. Slightly lift tag (1-2mm) then place back down
5. Move tag around reader surface
6. Quickly remove and replace tag
7. Remove tag completely and wait 2 seconds

### Expected Results
- No false "RFID removed" messages while tag is present
- System should tolerate brief disconnections
- "RFID Tag removal detected" should appear only when starting removal
- "RFID Tag removed - confirmed" should appear after 1 second delay
- No state changes until tag is actually removed

### Serial Output Examples
```
RFID Tag detected: A1B2C3D4
State: RFID Detected
State: Waiting for button press
RFID Tag removal detected - starting verification...
RFID Tag still present - canceling removal
RFID Tag removal detected - starting verification...
RFID Tag removed - confirmed
State: Idle (RFID removed)
```

## Test 3: State Machine Behavior

### Objective
Verify proper state transitions and system behavior.

### Procedure
1. Start in idle state (no RFID, LED slow blink)
2. Place RFID tag (should go to waiting state, LED solid)
3. Press button to start recording (LED fast blink)
4. Release button to stop recording (LED solid)
5. Long press for playback (LED slow blink)
6. Remove RFID tag during each state

### Expected Results
- Clear state transitions logged
- LED patterns match documented behavior
- Recording/playback stops when RFID removed
- System returns to idle when RFID removed

## Test 4: Serial Commands

### Objective
Verify serial interface functionality.

### Procedure
Send each command via serial monitor:
- `status`
- `files`
- `help`
- `delete audio_test.wav` (if file exists)

### Expected Results
- Status shows current system information
- Files lists audio files on SD card
- Help shows available commands
- Delete removes specified files

## Test 5: Audio File Management

### Objective
Verify audio recording and file handling.

### Procedure
1. Use different RFID tags to create multiple recordings
2. Check SD card contents
3. Verify WAV file format
4. Test file deletion via serial command

### Expected Results
- Unique filename for each RFID tag
- Proper WAV file header
- Files playable on computer
- Serial commands work for file management

## Performance Benchmarks

### Button Response Time
- Press to recognition: < 50ms after debounce
- Release to stop: < 50ms
- Long press detection: 1000ms ± 50ms

### RFID Detection Timing
- Tag placement to detection: < 500ms
- False removal protection: 1000ms delay
- Actual removal confirmation: 1000ms after last detection

### System Stability
- No memory leaks during extended operation
- Stable operation for 1+ hours
- Recovery from error states within 5 seconds

## Common Issues and Solutions

### Button Not Working
- Check GPIO 4 connection
- Verify pull-up resistor enabled
- Look for debounce messages in serial

### RFID Instability
- Check SPI connections
- Verify 3.3V power supply
- Try different RFID tags
- Adjust RFID_REMOVAL_DELAY if needed

### Audio Problems
- Check I2S pin connections
- Verify SD card format (FAT32)
- Ensure adequate power supply
- Check WAV file headers

## Test Report Template

```
Test Date: ___________
ESP32 Board: ___________
Software Version: ___________

Test 1 - Button Detection: PASS/FAIL
Notes: ___________

Test 2 - RFID Stability: PASS/FAIL
Notes: ___________

Test 3 - State Machine: PASS/FAIL
Notes: ___________

Test 4 - Serial Commands: PASS/FAIL
Notes: ___________

Test 5 - Audio Files: PASS/FAIL
Notes: ___________

Overall System Performance: EXCELLENT/GOOD/NEEDS_IMPROVEMENT

Issues Found:
___________

Recommendations:
___________
```