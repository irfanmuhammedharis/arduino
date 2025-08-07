# Hardware Setup Guide

## Wiring Diagram

```
ESP32 Development Board
┌─────────────────────────────────────┐
│  GPIO Pins                          │
│                                     │
│  GPIO 2  ──► LED (Status)           │
│  GPIO 4  ──► Button                 │
│  GPIO 5  ──► RC522 SDA              │
│  GPIO 15 ──► SD Card CS             │
│  GPIO 18 ──► SPI SCK (Shared)       │
│  GPIO 19 ──► SPI MISO (Shared)      │
│  GPIO 22 ──► RC522 RST              │
│  GPIO 23 ──► SPI MOSI (Shared)      │
│  GPIO 25 ──► I2S WS                 │
│  GPIO 26 ──► I2S SCK                │
│  GPIO 33 ──► I2S SD                 │
│                                     │
│  3.3V    ──► Power Supply           │
│  GND     ──► Ground                 │
└─────────────────────────────────────┘
```

## Component Connections

### RC522 RFID Reader Module
```
RC522    →    ESP32
──────────────────────
VCC      →    3.3V
GND      →    GND
SDA      →    GPIO 5
SCK      →    GPIO 18
MOSI     →    GPIO 23
MISO     →    GPIO 19
RST      →    GPIO 22
IRQ      →    Not Connected
```

### MicroSD Card Module
```
SD Card  →    ESP32
──────────────────────
VCC      →    3.3V
GND      →    GND
CS       →    GPIO 15
SCK      →    GPIO 18 (shared with RC522)
MOSI     →    GPIO 23 (shared with RC522)
MISO     →    GPIO 19 (shared with RC522)
```

### INMP441 I2S Microphone
```
INMP441  →    ESP32
──────────────────────
VDD      →    3.3V
GND      →    GND
SCK      →    GPIO 26
WS       →    GPIO 25
SD       →    GPIO 33
L/R      →    GND (for left channel)
```

### Push Button
```
Button   →    ESP32
──────────────────────
Pin 1    →    GPIO 4
Pin 2    →    GND
```
*Note: Internal pull-up resistor is enabled in code*

### Status LED
```
LED      →    ESP32
──────────────────────
Anode    →    GPIO 2
Cathode  →    220Ω Resistor → GND
```

## Important Notes

1. **Power Supply**: Ensure your ESP32 has adequate power supply (500mA+) as the system uses multiple peripherals simultaneously.

2. **SPI Bus Sharing**: The RC522 RFID reader and SD card module share the same SPI bus (GPIO 18, 19, 23). The code handles this properly by using different CS pins.

3. **SD Card Format**: Format your microSD card as FAT32 before use.

4. **RFID Tags**: Compatible with MIFARE Classic 1K/4K, MIFARE Ultralight, and other ISO14443A tags.

5. **Audio Quality**: For best audio quality, keep microphone wires short and away from high-frequency signals.

## Breadboard Layout Tips

1. Use the power rails for 3.3V and GND distribution
2. Keep digital signal wires away from power wires
3. Use short, direct connections where possible
4. Group related components together
5. Double-check all connections before powering on

## Testing Your Setup

1. Connect serial monitor at 115200 baud
2. Power on the ESP32
3. Look for initialization messages
4. Place an RFID tag near the reader
5. Check LED status changes
6. Test button press functionality
7. Verify SD card file creation

## Troubleshooting Hardware Issues

### SD Card Not Detected
- Check CS pin connection (GPIO 15)
- Verify SD card is FAT32 formatted
- Ensure adequate power supply
- Try a different SD card

### RFID Not Working
- Verify SPI connections (especially SCK, MOSI, MISO)
- Check RST pin connection (GPIO 22)
- Ensure 3.3V power to RC522
- Try different RFID tags

### Button Not Responding
- Check connection to GPIO 4
- Verify button is normally open
- Check ground connection
- Look for button press messages in serial output

### Audio Issues
- Verify I2S connections (GPIO 25, 26, 33)
- Check microphone power supply
- Ensure L/R pin is connected to GND
- Try adjusting microphone placement