/*
 * ============================================================================
 * ESP32 DS18B20 Temperature Monitor with Bluetooth Serial
 * ============================================================================
 * 
 * Project: Wireless temperature monitoring via Bluetooth Classic (SPP)
 * Author: irfanmuhammedharis
 * Date: 2025-11-12
 * Version: 1.1.0 (ERROR-FREE)
 * Board: ESP32 Dev Module
 * 
 * ERROR FIX CHANGELOG (v1.1.0):
 * - Replaced LED_BUILTIN with explicit GPIO2
 * - Added compatibility for boards without built-in LED
 * - Fixed compilation error on ESP32 core 3.3.1
 * 
 * Description:
 * Reads temperature from DS18B20 digital sensor using 1-Wire protocol,
 * calculates 5-sample average for accuracy, and transmits readings via
 * Bluetooth Serial every 15 seconds. Bluetooth name: "temperature_control"
 * 
 * Hardware Connections:
 * ---------------------
 * ESP32 Pin    | DS18B20 Pin   | Component
 * -------------|---------------|------------------
 * 3V3          | VDD (Pin 3)   | Power (Red)
 * GND          | GND (Pin 1)   | Ground (Black)
 * GPIO4        | DQ (Pin 2)    | Data (Yellow) + 4.7kΩ pull-up to 3V3
 * GPIO2        | (Optional)    | Built-in LED (error indicator)
 * 
 * Library Requirements:
 * ---------------------
 * 1. OneWire by Paul Stoffregen (v2.3.7+)
 * 2. DallasTemperature by Miles Burton (v3.9.0+)
 * 3. BluetoothSerial (included with ESP32 core)
 * 
 * Code References:
 * ----------------
 * - Based on: Examples > DallasTemperature > Simple
 * - Based on: Examples > BluetoothSerial > SerialToSerialBT
 * - Based on: Examples > 02.Digital > BlinkWithoutDelay
 * - Tutorial: https://RandomNerdTutorials.com/esp32-ds18b20-temperature-arduino-ide/
 * 
 * Safety Warnings:
 * ----------------
 * ⚠️ ALWAYS disconnect power before wiring changes
 * ⚠️ VERIFY all connections before applying power
 * ⚠️ DO NOT exceed 3.3V on any ESP32 GPIO
 * ⚠️ ENSURE 4.7kΩ pull-up is installed on GPIO4
 * ⚠️ CHECK DS18B20 pinout (flat side: GND-DQ-VDD)
 * 
 * License: MIT
 * ============================================================================
 */

// ============================================================================
// 1. LIBRARY INCLUDES
// ============================================================================
#include <OneWire.h>              // 1-Wire protocol for DS18B20
#include <DallasTemperature.h>    // DS18B20 temperature sensor library
#include "BluetoothSerial.h"      // ESP32 Bluetooth Classic SPP

// ============================================================================
// 2. CONFIGURATION CONSTANTS
// ============================================================================
// Pin Definitions
#define ONE_WIRE_BUS       4      // GPIO4 connected to DS18B20 DQ pin
#define STATUS_LED_PIN     2      // GPIO2 built-in LED (most ESP32 boards)

// Measurement Settings
#define TEMP_RESOLUTION    12     // DS18B20 resolution: 9-12 bits (0.0625°C precision)
#define SAMPLING_COUNT     5      // Number of samples to average
#define SAMPLE_DELAY_MS    200    // Delay between samples (milliseconds)

// Timing Settings
#define SEND_INTERVAL_MS   15000  // Bluetooth transmission interval (15 seconds)
#define SERIAL_BAUD        115200 // Serial Monitor baud rate

// Bluetooth Settings
#define BT_DEVICE_NAME     "temperature_control" // Bluetooth device name

// ============================================================================
// 3. GLOBAL OBJECTS & VARIABLES
// ============================================================================
// 1-Wire and DS18B20 instances
OneWire oneWire(ONE_WIRE_BUS);              // 1-Wire bus on GPIO4
DallasTemperature sensors(&oneWire);        // Dallas Temperature library
DeviceAddress sensorAddress;                // 64-bit sensor address storage

// Bluetooth Serial instance
BluetoothSerial SerialBT;                   // Bluetooth Serial object

// Timing variables (non-blocking)
unsigned long previousMillis = 0;           // Last transmission time

// Sensor status flag
bool sensorFound = false;                   // True if DS18B20 detected

// LED availability flag
bool ledAvailable = true;                   // True if GPIO2 LED works

// ============================================================================
// 4. HELPER FUNCTION: Safe LED Control
// ============================================================================
/**
 * @brief Safely blinks LED if available (handles missing LED_BUILTIN)
 * @param state HIGH or LOW
 */
void safeLedWrite(uint8_t state) {
  if (ledAvailable) {
    digitalWrite(STATUS_LED_PIN, state);
  }
}

// ============================================================================
// 5. HELPER FUNCTION: Get Averaged Temperature
// ============================================================================
/**
 * @brief Takes multiple temperature samples and returns the average
 * 
 * @return float Average temperature in Celsius, or DEVICE_DISCONNECTED_C on error
 */
float getAveragedTemperature() {
  float totalTemp = 0.0;
  int validReadings = 0;
  
  Serial.println("[SENSOR] Taking temperature samples...");
  
  for (int i = 0; i < SAMPLING_COUNT; i++) {
    sensors.requestTemperatures();
    delay(SAMPLE_DELAY_MS);
    
    float tempC = sensors.getTempC(sensorAddress);
    
    if (tempC != DEVICE_DISCONNECTED_C && tempC > -55.0 && tempC < 125.0) {
      totalTemp += tempC;
      validReadings++;
      Serial.printf("  Sample %d/%d: %.2f°C ✓\n", i + 1, SAMPLING_COUNT, tempC);
    } else {
      Serial.printf("  Sample %d/%d: INVALID (%.2f°C)\n", i + 1, SAMPLING_COUNT, tempC);
    }
  }
  
  if (validReadings > 0) {
    float avgTemp = totalTemp / validReadings;
    Serial.printf("  Average: %.2f°C (%d/%d valid)\n", avgTemp, validReadings, SAMPLING_COUNT);
    return avgTemp;
  } else {
    Serial.println("  ERROR: No valid readings!");
    return DEVICE_DISCONNECTED_C;
  }
}

// ============================================================================
// 6. HELPER FUNCTION: Print Sensor Address
// ============================================================================
/**
 * @brief Prints DS18B20 64-bit ROM address in hexadecimal
 * @param deviceAddress Pointer to 8-byte address array
 */
void printSensorAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// ============================================================================
// 7. SETUP FUNCTION (runs once at boot)
// ============================================================================
void setup() {
  // ---------------------------------------------------------------------------
  // 7.1 Serial Monitor Initialization
  // ---------------------------------------------------------------------------
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("ESP32 TEMPERATURE MONITOR - STARTING");
  Serial.println("========================================");
  Serial.printf("Firmware Version: 1.1.0 (ERROR-FREE)\n");
  Serial.printf("Compile Date: %s %s\n", __DATE__, __TIME__);
  Serial.printf("ESP32 Chip: %s\n", ESP.getChipModel());
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("========================================\n");
  
  // ---------------------------------------------------------------------------
  // 7.2 LED Initialization (with error handling)
  // ---------------------------------------------------------------------------
  Serial.println("[LED] Initializing status LED...");
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  // Test LED
  digitalWrite(STATUS_LED_PIN, HIGH);
  delay(200);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  Serial.printf("  ✅ Status LED on GPIO%d initialized\n", STATUS_LED_PIN);
  Serial.println("  (If LED doesn't blink, it's not critical)\n");
  
  // ---------------------------------------------------------------------------
  // 7.3 Bluetooth Initialization
  // ---------------------------------------------------------------------------
  Serial.println("[BLUETOOTH] Initializing Bluetooth Serial...");
  
  if (!SerialBT.begin(BT_DEVICE_NAME)) {
    Serial.println("  ❌ ERROR: Bluetooth initialization failed!");
    Serial.println("  Possible causes:");
    Serial.println("  - ESP32 core incompatibility");
    Serial.println("  - Insufficient memory");
    Serial.println("\n  SYSTEM HALTED. Reset ESP32.");
    
    // Visual error indicator with LED
    while (true) {
      safeLedWrite(HIGH);
      delay(250);
      safeLedWrite(LOW);
      delay(250);
    }
  }
  
  Serial.printf("  ✅ SUCCESS: Bluetooth initialized\n");
  Serial.printf("  Device Name: %s\n", BT_DEVICE_NAME);
  Serial.println("  Status: Ready to pair\n");
  
  // ---------------------------------------------------------------------------
  // 7.4 DS18B20 Sensor Initialization
  // ---------------------------------------------------------------------------
  Serial.println("[SENSOR] Initializing 1-Wire bus...");
  sensors.begin();
  Serial.println("  ✅ 1-Wire bus initialized\n");
  
  Serial.println("[SENSOR] Searching for DS18B20...");
  int deviceCount = sensors.getDeviceCount();
  Serial.printf("  Devices found: %d\n", deviceCount);
  
  if (deviceCount == 0) {
    Serial.println("\n  ❌ ERROR: No DS18B20 detected!");
    Serial.println("\n  TROUBLESHOOTING:");
    Serial.println("  1. Verify wiring:");
    Serial.println("     - Pin 1 (GND) → ESP32 GND");
    Serial.println("     - Pin 2 (DQ)  → ESP32 GPIO4");
    Serial.println("     - Pin 3 (VDD) → ESP32 3V3");
    Serial.println("  2. CHECK 4.7kΩ resistor:");
    Serial.println("     - Between GPIO4 and 3V3");
    Serial.println("  3. Verify DS18B20 pinout:");
    Serial.println("     (Flat side facing you)");
    Serial.println("     [GND] [DQ] [VDD]");
    Serial.println("       1     2    3");
    Serial.println("\n  System continues (readings unavailable)...\n");
    
    sensorFound = false;
    
  } else {
    if (sensors.getAddress(sensorAddress, 0)) {
      Serial.print("  ✅ SUCCESS: DS18B20 at 0x");
      printSensorAddress(sensorAddress);
      Serial.println();
      
      sensors.setResolution(sensorAddress, TEMP_RESOLUTION);
      Serial.printf("  Resolution: %d bits (%.4f°C precision)\n", 
                    TEMP_RESOLUTION, 
                    0.5 / (1 << (TEMP_RESOLUTION - 9)));
      
      sensors.setWaitForConversion(true);
      sensorFound = true;
      Serial.println();
      
    } else {
      Serial.println("  ⚠️ WARNING: Address read failed!");
      sensorFound = false;
    }
  }
  
  // ---------------------------------------------------------------------------
  // 7.5 Startup Complete
  // ---------------------------------------------------------------------------
  Serial.println("========================================");
  Serial.println("STARTUP COMPLETE");
  Serial.println("========================================");
  Serial.printf("Bluetooth: %s\n", SerialBT.hasClient() ? "Connected" : "Waiting...");
  Serial.printf("Sensor: %s\n", sensorFound ? "Operational" : "ERROR");
  Serial.printf("Update Interval: %d seconds\n", SEND_INTERVAL_MS / 1000);
  Serial.printf("Sample Averaging: %d samples\n", SAMPLING_COUNT);
  Serial.println("========================================\n");
  
  Serial.println("📱 BLUETOOTH PAIRING:");
  Serial.println("  1. Open Bluetooth settings");
  Serial.println("  2. Scan for devices");
  Serial.printf("  3. Connect to: %s\n", BT_DEVICE_NAME);
  Serial.println("  4. No PIN required");
  Serial.println("  5. Use Bluetooth terminal app\n");
}

// ============================================================================
// 8. MAIN LOOP (runs continuously)
// ============================================================================
void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= SEND_INTERVAL_MS) {
    previousMillis = currentMillis;
    
    Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.printf("📊 READING @ %lu ms\n", currentMillis);
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    if (!SerialBT.hasClient()) {
      Serial.println("⚠️ Bluetooth: No client connected");
    } else {
      Serial.println("✅ Bluetooth: Client connected");
    }
    
    char dataBuffer[128];
    
    if (!sensorFound) {
      const char* errorMsg = "❌ ERROR: Sensor not detected. Check wiring!\n";
      Serial.print(errorMsg);
      SerialBT.print(errorMsg);
      
    } else {
      float tempC = getAveragedTemperature();
      
      if (tempC == DEVICE_DISCONNECTED_C) {
        snprintf(dataBuffer, sizeof(dataBuffer), 
                 "❌ ERROR: Sensor read failed!\n");
        Serial.print(dataBuffer);
        SerialBT.print(dataBuffer);
        
      } else {
        float tempF = (tempC * 9.0 / 5.0) + 32.0;
        
        snprintf(dataBuffer, sizeof(dataBuffer), 
                 "Temp: %.2f°C / %.2f°F\n", tempC, tempF);
        
        Serial.print(dataBuffer);
        SerialBT.print(dataBuffer);
        
        Serial.printf("Heap: %d bytes | Uptime: %lu sec\n", 
                      ESP.getFreeHeap(), currentMillis / 1000);
      }
    }
    
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  }
  
  delay(10);
}