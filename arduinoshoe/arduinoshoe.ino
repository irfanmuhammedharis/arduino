/*
 * =============================================================================
 * File:         BatteryMonitor_BT_Fixed.ino
 * Description: Battery voltage monitor with reliable Bluetooth single-line output
 * Board:       Arduino Uno/Nano (ATmega328P @ 5V, 16MHz)
 * 
 * Hardware Connections:
 *   - A0:   Voltage divider output (2: 1 ratio for 0-10V input → 0-5V ADC)
 *   - D10: HC-05/HC-06 TX → Arduino RX (SoftwareSerial)
 *   - D11: HC-05/HC-06 RX ← Arduino TX (use 1K/2K voltage divider for 3.3V BT!)
 *   - GND:  Common ground between Arduino and BT module
 *   - 5V:   BT module VCC (check module specs - some need 3.3V regulator)
 * 
 * Based on:
 *   - Examples > 03. Analog > AnalogInput (ADC reading)
 *   - Examples > 04.Communication > SoftwareSerialExample
 *   - Examples > 02.Digital > BlinkWithoutDelay (non-blocking timing)
 * 
 * Library:      SoftwareSerial (built-in)
 * Reference:   https://github.com/arduino-libraries/SoftwareSerial
 * 
 * Safety: 
 *   - Never exceed 5V on A0 pin
 *   - Use voltage divider on BT RX if module is 3.3V logic
 * =============================================================================
 */

#include <SoftwareSerial.h>

// ==================== PIN DEFINITIONS ====================
#define BT_RX_PIN       10      // Arduino RX ← BT TX
#define BT_TX_PIN       11      // Arduino TX → BT RX (with level shifter if needed)
#define BATTERY_PIN     A0      // Voltage divider output

// ==================== CONFIGURATION ====================
#define DIVIDER_RATIO   2.0     // Voltage divider ratio (R1+R2)/R2
#define BATTERY_EMPTY   5.5     // Minimum voltage (0%)
#define BATTERY_FULL    8.0     // Maximum voltage (100%)
#define TX_INTERVAL_MS  10000   // Transmission interval (10 seconds)
#define ADC_SAMPLES     20      // Number of ADC samples for averaging
#define ADC_DELAY_MS    2       // Delay between ADC samples
#define BT_BAUD_RATE    9600    // Bluetooth baud rate
#define CHAR_DELAY_US   500     // Microseconds delay between characters

// ==================== GLOBAL OBJECTS ====================
SoftwareSerial BT(BT_RX_PIN, BT_TX_PIN);

// ==================== CALIBRATION ====================
float calibrationFactor = 1.0;  // Adjust based on multimeter comparison

// ==================== TIMING ====================
unsigned long lastTxTime = 0;

// ==================== FUNCTION:  Read Averaged Voltage ====================
float readVoltage() {
  long sum = 0;
  
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(BATTERY_PIN);
    delay(ADC_DELAY_MS);
  }

  float adc = sum / (float)ADC_SAMPLES;
  float rawVoltage = (adc / 1023.0) * 5.0;
  return rawVoltage * DIVIDER_RATIO * calibrationFactor;
}

// ==================== FUNCTION:  Send String Char-by-Char ====================
// This ensures each character is transmitted completely before the next
void sendBTString(const char* str) {
  while (*str) {
    BT.write(*str++);
    delayMicroseconds(CHAR_DELAY_US);  // Allow BT buffer to process
  }
  BT.flush();  // Wait for all data to be transmitted
}

// ==================== FUNCTION:  Send Battery Data ====================
void sendBatteryData(float voltage, int percent) {
  char vStr[8];
  char msg[32];
  
  // Convert float to string (AVR-safe method)
  dtostrf(voltage, 4, 2, vStr);
  
  // Format message - CRITICAL: Use println equivalent manually
  // Option 1: Compact format
  snprintf(msg, sizeof(msg), "V:%s B:%d%%", vStr, percent);
  
  // Send with guaranteed single-line delivery
  sendBTString(msg);
  
  // Send line terminator separately with delay
  delay(10);
  BT.write('\r');
  delayMicroseconds(CHAR_DELAY_US);
  BT.write('\n');
  BT.flush();
}

// ==================== SETUP ====================
void setup() {
  // Initialize Serial Monitor for debugging
  Serial. begin(9600);
  while (!Serial && millis() < 3000);  // Wait up to 3s for Serial (USB boards)
  
  // Initialize Bluetooth
  BT.begin(BT_BAUD_RATE);
  
  // Startup delay for BT module initialization
  delay(1000);
  
  // Send startup message
  Serial.println(F("Battery Monitor v1.0 - Starting..."));
  Serial.println(F("Bluetooth transmission every 10 seconds"));
  
  // Initial BT test message
  sendBTString("READY");
  delay(10);
  BT.write('\r');
  BT.write('\n');
  BT.flush();
  
  delay(500);
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long currentMillis = millis();
  
  // Non-blocking timing check
  if (currentMillis - lastTxTime < TX_INTERVAL_MS) {
    return;
  }
  lastTxTime = currentMillis;

  // Read battery voltage
  float voltage = readVoltage();

  // Calculate percentage
  int percent = map(
    (long)(voltage * 100),
    (long)(BATTERY_EMPTY * 100),
    (long)(BATTERY_FULL * 100),
    0,
    100
  );
  percent = constrain(percent, 0, 100);

  // Send to Bluetooth (single line guaranteed)
  sendBatteryData(voltage, percent);

  // Debug output to Serial Monitor
  Serial.print(F("TX → BT: V: "));
  Serial.print(voltage, 2);
  Serial.print(F(" B:"));
  Serial.print(percent);
  Serial.println(F("%"));
}