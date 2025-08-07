# Hardware Setup Guide

## Circuit Connections

### ESP32 Development Board Pin Configuration

```
ESP32 Pin    | Component         | Purpose
-------------|-------------------|------------------
GPIO 2       | LED (Built-in)    | Status indicator
GPIO 21      | Push Button       | Control input (with pull-up)
GPIO 4       | RC522 SS          | RFID chip select
GPIO 13      | RC522 MOSI        | RFID data input
GPIO 12      | RC522 MISO        | RFID data output
GPIO 16      | RC522 SCK         | RFID clock
GPIO 22      | RC522 RST         | RFID reset
GPIO 5       | SD Card CS        | SD card chip select
GPIO 23      | SD Card MOSI      | SD card data input
GPIO 19      | SD Card MISO      | SD card data output
GPIO 18      | SD Card SCK       | SD card clock
GPIO 25      | INMP441 WS        | I2S word select (recording)
GPIO 26      | INMP441 SCK       | I2S clock (recording)
GPIO 27      | INMP441 SD        | I2S data (recording)
GPIO 32      | Speaker WS        | I2S word select (playback)
GPIO 14      | Speaker SCK       | I2S clock (playback)
GPIO 33      | Speaker DATA      | I2S data (playback)
3.3V         | All modules VCC   | Power supply
GND          | All modules GND   | Ground reference
```

## Component Specifications

### INMP441 I2S Microphone Module
- **Supply Voltage**: 3.3V
- **Interface**: I2S Digital
- **Frequency Response**: 60Hz - 15kHz
- **SNR**: 61dB
- **Sensitivity**: -26dBFS

**Connections:**
```
INMP441    ESP32
--------   -----
VDD    ->  3.3V
GND    ->  GND
SD     ->  GPIO 27
WS     ->  GPIO 25
SCK    ->  GPIO 26
L/R    ->  GND (for left channel)
```

### I2S Speaker/Amplifier Module
- **Supply Voltage**: 3.3V - 5V
- **Interface**: I2S Digital
- **Output Power**: Depends on amplifier module
- **Common modules**: MAX98357A, UDA1334A

**Connections:**
```
I2S Module   ESP32
----------   -----
VIN      ->  3.3V or 5V
GND      ->  GND
DIN      ->  GPIO 33
BCLK     ->  GPIO 14
LRC      ->  GPIO 32
GAIN     ->  GND (for 9dB gain)
SD       ->  3.3V (enable)
```

### RC522 RFID Module
- **Supply Voltage**: 3.3V
- **Interface**: SPI
- **Frequency**: 13.56MHz
- **Read Range**: ~3cm

**Connections:**
```
RC522    ESP32
-----    -----
3.3V ->  3.3V
RST  ->  GPIO 22
GND  ->  GND
IRQ  ->  Not connected
MISO ->  GPIO 12
MOSI ->  GPIO 13
SCK  ->  GPIO 16
SDA  ->  GPIO 4
```

### MicroSD Card Module
- **Supply Voltage**: 3.3V or 5V
- **Interface**: SPI
- **Supported**: FAT16/FAT32
- **Capacity**: Up to 32GB recommended

**Connections:**
```
SD Module  ESP32
---------  -----
VCC    ->  3.3V
GND    ->  GND
MISO   ->  GPIO 19
MOSI   ->  GPIO 23
SCK    ->  GPIO 18
CS     ->  GPIO 5
```

### Push Button
- **Type**: Momentary push button
- **Configuration**: Active LOW with internal pull-up
- **Debouncing**: Handled in software (50ms)

**Connections:**
```
Button   ESP32
------   -----
One pin -> GPIO 21
Other pin -> GND
```

## Power Supply Requirements

### Power Consumption Analysis
- **ESP32**: ~240mA (WiFi active), ~80mA (WiFi off)
- **INMP441**: ~1.4mA
- **RC522**: ~13mA (idle), ~26mA (active)
- **SD Card**: ~100mA (write), ~40mA (read)
- **I2S Amplifier**: ~2-10mA (depends on volume)

**Total estimated consumption**: 200-400mA @ 3.3V

### Recommended Power Supply
- **USB Power**: 5V/1A USB adapter
- **Battery**: 3.7V Li-Po 2000mAh+ for portable operation
- **Regulation**: ESP32 dev board includes 3.3V regulator

## PCB Layout Considerations

### Signal Integrity
1. **I2S Lines**: Keep clock and data lines short and parallel
2. **SPI Lines**: Use proper termination, avoid long runs
3. **Power Planes**: Separate analog and digital grounds if possible
4. **Decoupling**: Place 100nF capacitors near each IC

### EMI Considerations
1. **RFID Antenna**: Keep away from switching circuits
2. **Audio Lines**: Shield from digital switching noise
3. **Crystal**: Keep ESP32 crystal traces short
4. **Power Supply**: Use proper filtering

## Assembly Instructions

### Step 1: Prepare Components
1. ESP32 development board
2. Breadboard or PCB
3. Jumper wires
4. All modules and components

### Step 2: Power Connections
1. Connect 3.3V rail to all module VCC pins
2. Connect GND rail to all module GND pins
3. Verify voltage levels with multimeter

### Step 3: SPI Connections
1. Connect SD card SPI (VSPI pins)
2. Connect RC522 SPI (HSPI pins)
3. Ensure proper chip select isolation

### Step 4: I2S Connections
1. Connect INMP441 to I2S0 pins
2. Connect speaker module to I2S1 pins
3. Verify clock and data line integrity

### Step 5: Control Connections
1. Connect button between GPIO21 and GND
2. LED is typically built-in on ESP32 boards

### Step 6: Testing
1. Upload test sketch
2. Verify serial output
3. Test each component individually

## Troubleshooting Hardware Issues

### No Serial Output
- Check USB cable and drivers
- Verify ESP32 power LED
- Try different baud rates

### SD Card Not Detected
- Check SPI wiring
- Verify SD card format (FAT32)
- Test with different SD card
- Check power supply stability

### RFID Not Working
- Verify 3.3V power supply
- Check SPI connections
- Test with known working RFID tag
- Verify antenna integrity

### Audio Issues
- Check I2S wiring
- Verify component power supplies
- Test with different audio components
- Check for proper grounding

### Power Issues
- Measure actual current consumption
- Check for short circuits
- Verify regulator capacity
- Test with external power supply

## Safety Considerations

### Electrical Safety
1. Always disconnect power when wiring
2. Verify connections before powering on
3. Use proper ESD precautions
4. Don't exceed component voltage ratings

### Component Protection
1. Use current-limiting resistors where needed
2. Protect against reverse polarity
3. Use proper decoupling capacitors
4. Avoid static discharge

## Performance Optimization

### Signal Quality
1. Use short, direct connections
2. Minimize crosstalk between signals
3. Use proper ground planes
4. Shield sensitive analog circuits

### Power Efficiency
1. Use sleep modes when appropriate
2. Optimize software for power consumption
3. Use efficient power supplies
4. Implement proper power management

## Testing and Validation

### Functional Tests
1. **Power-on Test**: Verify all components receive power
2. **Communication Test**: Check SPI and I2S interfaces
3. **Audio Test**: Record and play back test audio
4. **RFID Test**: Detect and read RFID tags
5. **Integration Test**: Verify complete system operation

### Performance Tests
1. **Audio Quality**: Check frequency response and noise
2. **RFID Range**: Test detection distance
3. **File System**: Test SD card read/write speed
4. **Power Consumption**: Measure actual power usage
5. **Reliability**: Long-term operation test