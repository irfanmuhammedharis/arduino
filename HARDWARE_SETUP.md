# Hardware Setup Guide - ESP32 RFID Audio Recorder

## 🔌 Complete Wiring Diagram

```
                    ESP32 Development Board
    ┌─────────────────────────────────────────────────┐
    │                                                 │
    │  ┌─────────────────────────────────────────┐    │
    │  │           GPIO PINS                     │    │
    │  │                                         │    │
    │  │  GPIO 2  ──► LED (Status)               │    │
    │  │  GPIO 4  ──► (Reserved)                 │    │
    │  │  GPIO 5  ──► RC522 SDA                  │    │
    │  │  GPIO 12 ──► I2S Playback SD            │    │
    │  │  GPIO 14 ──► I2S Playback SCK           │    │
    │  │  GPIO 15 ──► SD Card CS                 │    │
    │  │  GPIO 18 ──► SPI SCK (Shared)           │    │
    │  │  GPIO 19 ──► SPI MISO (Shared)          │    │
    │  │  GPIO 21 ──► Button                     │    │
    │  │  GPIO 22 ──► RC522 RST                  │    │
    │  │  GPIO 23 ──► SPI MOSI (Shared)          │    │
    │  │  GPIO 25 ──► I2S Recording WS           │    │
    │  │  GPIO 26 ──► I2S Recording SCK          │    │
    │  │  GPIO 32 ──► I2S Playback WS            │    │
    │  │  GPIO 33 ──► I2S Recording SD           │    │
    │  │                                         │    │
    │  │  3.3V    ──► Power Supply               │    │
    │  │  GND     ──► Ground                     │    │
    │  └─────────────────────────────────────────┘    │
    └─────────────────────────────────────────────────┘
```

## 🏷️ RC522 RFID Reader Module

### Pin Connections
```
RC522 Module    ────────►    ESP32
──────────────────────────────────────
VCC (3.3V)      ────────►    3.3V
GND             ────────►    GND
SDA (SS)        ────────►    GPIO 5
SCK             ────────►    GPIO 18
MOSI            ────────►    GPIO 23
MISO            ────────►    GPIO 19
RST             ────────►    GPIO 22
IRQ             ────────►    Not Connected
```

### Important Notes
- **Power**: Must use 3.3V, NOT 5V (will damage module)
- **SPI Bus**: Shares bus with SD card module
- **Range**: Optimal reading distance is 0-3cm
- **Tags**: Compatible with MIFARE Classic 1K/4K, NTAG213/215/216

## 💾 MicroSD Card Module

### Pin Connections
```
SD Card Module  ────────►    ESP32
──────────────────────────────────────
VCC             ────────►    3.3V
GND             ────────►    GND
CS              ────────►    GPIO 15
SCK             ────────►    GPIO 18 (shared with RC522)
MOSI            ────────►    GPIO 23 (shared with RC522)
MISO            ────────►    GPIO 19 (shared with RC522)
```

### SD Card Requirements
- **Format**: FAT32 (required)
- **Capacity**: 1GB to 32GB recommended
- **Class**: Class 10 or higher for better performance
- **Brands**: SanDisk, Kingston, Samsung tested and working

## 🎤 I2S Microphone (INMP441)

### Pin Connections
```
INMP441         ────────►    ESP32
──────────────────────────────────────
VDD             ────────►    3.3V
GND             ────────►    GND
SCK (BCLK)      ────────►    GPIO 26
WS (LRCLK)      ────────►    GPIO 25
SD (DOUT)       ────────►    GPIO 33
L/R             ────────►    GND (Left channel)
```

### Audio Specifications
- **Sample Rate**: 16 kHz (voice optimized)
- **Bit Depth**: 16-bit
- **SNR**: 65 dB
- **Sensitivity**: -26 dBFS
- **Channel**: Mono (left channel selected)

## 🔊 I2S DAC/Amplifier (Optional - for Playback)

### Pin Connections
```
I2S DAC/Amp     ────────►    ESP32
──────────────────────────────────────
VIN             ────────►    3.3V
GND             ────────►    GND
BCK (BCLK)      ────────►    GPIO 14
WS (LRCLK)      ────────►    GPIO 32
DIN (DATA)      ────────►    GPIO 12
```

### Recommended Models
- **MAX98357A**: I2S amplifier with speaker output
- **PCM5102A**: High-quality I2S DAC
- **UDA1334A**: Simple I2S DAC breakout

## 🔘 Push Button & LED

### Button Connections
```
Push Button     ────────►    ESP32
──────────────────────────────────────
Pin 1           ────────►    GPIO 21
Pin 2           ────────►    GND
```
- **Type**: Momentary push button (normally open)
- **Internal Pull-up**: Enabled in software
- **Debouncing**: 50ms software debouncing implemented

### LED Connections
```
Status LED      ────────►    ESP32
──────────────────────────────────────
Anode (+)       ────────►    GPIO 2
Cathode (-)     ────────►    220Ω Resistor ──► GND
```
- **Type**: Standard 5mm LED (any color)
- **Current**: ~10mA through 220Ω resistor
- **Voltage**: 3.3V logic level

## ⚡ Power Requirements

### Power Consumption Analysis
```
Component           Current Draw    Notes
─────────────────────────────────────────────
ESP32 (Active)      ~80-100mA      WiFi disabled
RC522 RFID          ~13-26mA       Reading mode
SD Card Module      ~20-80mA       Read/write operations
INMP441 Mic         ~1.4mA         Continuous recording
I2S DAC (Optional)  ~10-30mA       During playback
LED                 ~10mA          When illuminated
─────────────────────────────────────────────
Total Peak:         ~150-250mA     Normal operation
Surge Current:      ~300-400mA     During initialization
```

### Power Supply Options
1. **USB Power**: 5V via USB cable (most common)
2. **Battery Pack**: 3x AA batteries (4.5V)
3. **Li-Po Battery**: 3.7V with voltage regulator
4. **External 5V**: DC adapter with adequate current rating

### Power Supply Requirements
- **Voltage**: 3.3V-5V input range
- **Current**: Minimum 500mA capacity
- **Regulation**: Stable voltage under load
- **Filtering**: Low ripple for audio quality

## 🔗 SPI Bus Sharing

### Critical Setup Notes
The RC522 RFID reader and SD Card module share the SPI bus. This requires careful handling:

```cpp
// SPI bus shared signals
SCK  (GPIO 18) ── Shared clock
MOSI (GPIO 23) ── Master Out, Slave In
MISO (GPIO 19) ── Master In, Slave Out

// Individual chip select signals
RC522_SS (GPIO 5)  ── RFID Reader select
SD_CS    (GPIO 15) ── SD Card select
```

### SPI Bus Guidelines
- **Timing**: Allow small delays between component access
- **Chip Select**: Only one device active at a time
- **Clock Speed**: 4MHz maximum for compatibility
- **Wire Length**: Keep SPI wires short (<15cm)

## 🧪 Assembly Instructions

### Step 1: Breadboard Layout
```
Power Rails (Top):    3.3V ────── GND
Power Rails (Bottom): 3.3V ────── GND

Left Side:   ESP32 + Button + LED
Center:      RC522 RFID Module
Right Side:  SD Card + INMP441 Microphone
```

### Step 2: Power Distribution
1. Connect ESP32 3.3V pin to breadboard power rail
2. Connect ESP32 GND pin to breadboard ground rail
3. Verify 3.3V on power rail with multimeter
4. Add 100µF capacitor across power rails for stability

### Step 3: SPI Connections
1. Wire SCK, MOSI, MISO to both RC522 and SD card
2. Connect individual CS pins (GPIO 5 for RC522, GPIO 15 for SD)
3. Add RST connection for RC522 (GPIO 22)
4. Test continuity with multimeter

### Step 4: I2S Connections
1. Wire INMP441 microphone I2S pins
2. If using playback, wire I2S DAC pins
3. Ensure power connections are secure
4. Check for short circuits

### Step 5: Control Connections
1. Connect button between GPIO 21 and GND
2. Connect LED with resistor to GPIO 2
3. Test button and LED functionality
4. Verify all connections match wiring diagram

## 🔍 Testing & Verification

### Basic Connectivity Test
```bash
# Use multimeter to verify:
1. 3.3V on all VCC pins
2. 0V on all GND pins
3. Continuity on shared SPI lines
4. No short circuits between power and ground
```

### Component Testing Procedure
```cpp
// Upload test sketch and monitor serial output:
1. Power LED should illuminate briefly during startup
2. Button test requires physical button press
3. RFID self-test should report "PASS"
4. SD card should show available space
5. I2S initialization should succeed
```

### Signal Quality Verification
- **SPI Signals**: Clean square waves, no ringing
- **I2S Clock**: Stable 16 kHz sample rate
- **Power Rails**: <50mV ripple under load
- **Ground**: Single point grounding, no loops

## ⚠️ Common Wiring Mistakes

### Power Issues
❌ **5V to RC522**: Will damage the module permanently
❌ **Insufficient current**: System resets or fails to initialize
❌ **Poor grounding**: Noise and instability

### SPI Problems
❌ **Missing pull-ups**: Intermittent communication failures
❌ **Long wires**: Signal integrity issues above 4MHz
❌ **Wrong CS pins**: Components won't respond

### I2S Issues
❌ **Wrong clock pins**: No audio recording
❌ **Missing ground**: Noisy or no audio
❌ **Incorrect L/R pin**: Wrong channel selected

## 🛠 Professional PCB Layout (Advanced)

### PCB Design Considerations
- **Layer Stack**: 2-layer minimum, 4-layer preferred
- **Trace Width**: 10 mil minimum for signals, 20 mil for power
- **Via Size**: 8 mil drill, 16 mil pad
- **Ground Plane**: Solid ground plane on bottom layer
- **Power Plane**: Dedicated 3.3V plane or thick traces

### Component Placement
- **Crystal**: Close to ESP32 with short traces
- **Bypass Capacitors**: Near each IC power pin
- **Analog Section**: Separate from digital switching
- **Antenna Area**: Keep clear of other components

### EMI Considerations
- **Shielding**: Optional RF shield for ESP32
- **Filtering**: Ferrite beads on power inputs
- **Grounding**: Star ground configuration
- **Isolation**: Digital and analog ground separation

## 📏 Mechanical Considerations

### Enclosure Requirements
- **Dimensions**: Minimum 100mm x 70mm x 30mm
- **Material**: Non-conductive plastic preferred
- **Ventilation**: Slots for heat dissipation
- **Access**: Holes for SD card, button, LED, USB

### RFID Reading Area
- **Antenna Position**: Top surface of enclosure
- **Material Thickness**: <3mm plastic maximum
- **Metal Interference**: Keep metal components away
- **Reading Zone**: Mark optimal tag placement area

### User Interface
- **Button**: Tactile feedback, weatherproof if needed
- **LED**: Visible indicator, diffused if in enclosure
- **SD Card**: Accessible for file management
- **Serial Port**: USB connector for debugging

This hardware setup guide provides all necessary information for building a robust, professional-grade ESP32 RFID Audio Recorder system.