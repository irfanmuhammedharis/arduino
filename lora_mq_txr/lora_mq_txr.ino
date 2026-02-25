/*
 * =============================================================================
 * PROJECT:      DC Voltage and Current Monitor
 * DESCRIPTION:  Measures DC voltage via resistor divider and current via ACS712
 * BOARD:       Arduino Uno/Nano/Mega (5V logic)
 * VERSION:     1.0
 * DATE:        2025-12-20
 * =============================================================================
 * 
 * HARDWARE CONNECTIONS:
 * ---------------------
 * Voltage Divider (for voltage measurement):
 *   - A0: Voltage divider output (Vin -> R1 -> A0 -> R2 -> GND)
 *   - R1 = 30kΩ (from Vin to A0)
 *   - R2 = 7.5kΩ (from A0 to GND)
 *   - Max input voltage: 25V (with these resistor values)
 * 
 * ACS712 Current Sensor (for current measurement):
 *   - A1: ACS712 analog output (Vout pin)
 *   - VCC: 5V
 *   - GND: GND
 * 
 * LIBRARY REQUIREMENTS:
 * ---------------------
 * None (uses built-in Arduino functions)
 * 
 * REFERENCES:
 * -----------
 * Based on:  Examples > 01. Basics > AnalogReadSerial
 * Based on: Examples > 03.Analog > AnalogInput
 * 
 * SAFETY WARNINGS:
 * ----------------
 * !  Do NOT exceed 25V on voltage divider input (with R1=30k, R2=7.5k)
 * ! Ensure proper isolation for high-voltage measurements
 * ! ACS712 is rated for specific current ranges - do not exceed! 
 * !  Add fuse protection for current measurement circuits
 * =============================================================================
 */

// =============================================================================
// PIN DEFINITIONS
// =============================================================================
#define VOLTAGE_PIN   A0    // Voltage divider input
#define CURRENT_PIN   A1    // ACS712 current sensor input

// =============================================================================
// VOLTAGE DIVIDER CONFIGURATION
// =============================================================================
const float R1 = 30000.0;           // Upper resistor (ohms) - Vin to ADC
const float R2 = 7500.0;            // Lower resistor (ohms) - ADC to GND
const float DIVIDER_RATIO = R2 / (R1 + R2);  // Voltage divider ratio

// =============================================================================
// ACS712 CURRENT SENSOR CONFIGURATION
// Select the appropriate scale factor for your ACS712 variant: 
// =============================================================================
// const float SCALE_FACTOR = 0.185;  // ACS712-05B (5A version)  - 185mV/A
const float SCALE_FACTOR = 0.100;     // ACS712-20A (20A version) - 100mV/A
// const float SCALE_FACTOR = 0.066;  // ACS712-30A (30A version) - 66mV/A

// =============================================================================
// ADC CONFIGURATION (Common to both measurements)
// =============================================================================
const float V_REF = 5.00;           // Arduino reference voltage (V)
const int ADC_RESOLUTION = 1024;    // 10-bit ADC resolution
const float ADC_STEP = V_REF / ADC_RESOLUTION;  // Voltage per ADC step
const float ZERO_POINT = V_REF / 2.0;           // ACS712 zero-current voltage

// =============================================================================
// SAMPLING CONFIGURATION
// =============================================================================
const int CURRENT_SAMPLES = 500;    // Number of samples for current averaging
const int VOLTAGE_SAMPLES = 10;     // Number of samples for voltage averaging
const unsigned long SAMPLE_DELAY_US = 200;  // Microseconds between samples

// =============================================================================
// TIMING CONFIGURATION (Non-blocking)
// =============================================================================
const unsigned long MEASUREMENT_INTERVAL_MS = 1000;  // Measurement interval
unsigned long previousMillis = 0;

// =============================================================================
// MEASUREMENT VARIABLES
// =============================================================================
float inputVoltage = 0.0;
float currentAmps = 0.0;
float powerWatts = 0.0;

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================
float readVoltage(void);
float readCurrent(void);
void printMeasurements(void);

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to connect (needed for Leonardo/Micro)
  }
  
  // Print startup header
  Serial.println(F("=============================================="));
  Serial.println(F("  DC Voltage & Current Monitor v1.0"));
  Serial.println(F("=============================================="));
  Serial.println(F("Hardware Configuration:"));
  Serial.print(F("  Voltage Divider:  R1="));
  Serial.print(R1/1000, 1);
  Serial.print(F("k, R2="));
  Serial.print(R2/1000, 1);
  Serial.println(F("k"));
  Serial.print(F("  Max Voltage: "));
  Serial.print(V_REF / DIVIDER_RATIO, 1);
  Serial.println(F("V"));
  Serial.print(F("  ACS712 Scale:  "));
  Serial.print(SCALE_FACTOR * 1000, 0);
  Serial.println(F(" mV/A"));
  Serial.println(F("=============================================="));
  Serial.println(F("Voltage(V)\tCurrent(A)\tPower(W)"));
  Serial.println(F("----------------------------------------------"));
  
  // Allow sensors to stabilize
  delay(500);
}

// =============================================================================
// MAIN LOOP (Non-blocking)
// =============================================================================
void loop() {
  unsigned long currentMillis = millis();
  
  // Non-blocking timing check
  if (currentMillis - previousMillis >= MEASUREMENT_INTERVAL_MS) {
    previousMillis = currentMillis;
    
    // Read sensors
    inputVoltage = readVoltage();
    currentAmps = readCurrent();
    
    // Calculate power (P = V × I)
    powerWatts = inputVoltage * currentAmps;
    
    // Display results
    printMeasurements();
  }
  
  // Other non-blocking tasks can be added here
}

// =============================================================================
// VOLTAGE READING FUNCTION
// Reads voltage through resistor divider with averaging
// =============================================================================
float readVoltage(void) {
  float adcSum = 0.0;
  
  // Take multiple samples for noise reduction
  for (int i = 0; i < VOLTAGE_SAMPLES; i++) {
    adcSum += analogRead(VOLTAGE_PIN);
    delayMicroseconds(SAMPLE_DELAY_US);
  }
  
  // Calculate average ADC value
  float adcAverage = adcSum / VOLTAGE_SAMPLES;
  
  // Convert ADC value to voltage at divider output
  float adcVoltage = adcAverage * ADC_STEP;
  
  // Calculate actual input voltage (before divider)
  float voltage = adcVoltage / DIVIDER_RATIO;
  
  // Clamp negative values to zero (noise floor)
  if (voltage < 0.05) {
    voltage = 0.0;
  }
  
  return voltage;
}

// =============================================================================
// CURRENT READING FUNCTION
// Reads current from ACS712 sensor with extensive averaging
// =============================================================================
float readCurrent(void) {
  float voutSum = 0.0;
  
  // Take many samples for precision (ACS712 output can be noisy)
  for (int i = 0; i < CURRENT_SAMPLES; i++) {
    voutSum += analogRead(CURRENT_PIN) * ADC_STEP;
    delayMicroseconds(SAMPLE_DELAY_US);
  }
  
  // Calculate average sensor voltage
  float voutAverage = voutSum / CURRENT_SAMPLES;
  
  // Convert voltage to current using scale factor
  // ACS712 outputs 2.5V at zero current (half of Vcc)
  float current = (voutAverage - ZERO_POINT) / SCALE_FACTOR;
  
  // Apply dead-band for noise (±0.05A)
  if (abs(current) < 0.05) {
    current = 0.0;
  }
  
  return current;
}

// =============================================================================
// OUTPUT FUNCTION
// Prints all measurements to Serial Monitor
// =============================================================================
void printMeasurements(void) {
  // Print voltage
  Serial.print(inputVoltage, 2);
  Serial.print(F(" V\t\t"));
  
  // Print current
  Serial.print(currentAmps, 2);
  Serial.print(F(" A\t\t"));
  
  // Print power
  Serial.print(powerWatts, 2);
  Serial.println(F(" W"));
}