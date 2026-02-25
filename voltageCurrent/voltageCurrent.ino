
#include <SoftwareSerial.h>

// =============================================================================
// PIN DEFINITIONS
// =============================================================================
#define VOLTAGE_PIN   A0    // Voltage divider input
#define CURRENT_PIN   A1    // ACS712 current sensor input

// Bluetooth SoftwareSerial pins (as specified by user)
#define BT_RX_PIN     2     // Arduino pin D3 <- Bluetooth TX (Arduino receives)
#define BT_TX_PIN     3     // Arduino pin D2 -> Bluetooth RX (Arduino transmits)

// =============================================================================
// BLUETOOTH CONFIGURATION
// =============================================================================
// Create SoftwareSerial object for Bluetooth communication
// SoftwareSerial(RX, TX) - RX receives from BT TX, TX sends to BT RX
SoftwareSerial BTSerial(BT_RX_PIN, BT_TX_PIN);

// Bluetooth baud rate (default for HC-05/HC-06 is 9600)
const long BT_BAUD_RATE = 9600;

// =============================================================================
// VOLTAGE DIVIDER CONFIGURATION
// =============================================================================
float R1 = 30000.0;         // Upper resistor (ohms) - Vin to ADC
float R2 = 7500.0;          // Lower resistor (ohms) - ADC to GND

// =============================================================================
// ACS712 CURRENT SENSOR CONFIGURATION
// Select the appropriate scale factor for your ACS712 variant: 
// =============================================================================
// const float SCALE_FACTOR = 0.185;  // ACS712-05B (5A version)  - 185mV/A
const float SCALE_FACTOR = 0.100;     // ACS712-20A (20A version) - 100mV/A
// const float SCALE_FACTOR = 0.066;  // ACS712-30A (30A version) - 66mV/A

// =============================================================================
// CALIBRATED ZERO-POINT OFFSET
// =============================================================================
// ⚡ IMPORTANT: This is the raw ADC value when NO current is flowing
// Measured from your specific ACS712 sensor (theoretical = 512, actual = 441)
const int ZERO_CURRENT_ADC_OFFSET = 440;  // Calibrated zero-current ADC reading

// =============================================================================
// ADC CONFIGURATION
// =============================================================================
float ref_voltage = 5.0;              // Arduino reference voltage (V)
const int ADC_RESOLUTION = 1024;      // 10-bit ADC resolution

// =============================================================================
// SAMPLING CONFIGURATION FOR CURRENT
// =============================================================================
const int CURRENT_SAMPLES = 1000;     // Number of samples for current averaging
const unsigned long SAMPLE_DELAY_MS = 1;  // Milliseconds between current samples

// =============================================================================
// MEASUREMENT VARIABLES
// =============================================================================
// Voltage measurement variables
int adc_value = 0;
float adc_voltage = 0.0;
float in_voltage = 0.0;

// Current measurement variables
float currentAmps = 0.0;
float sensorVoltage = 0.0;

// Power calculation
float powerWatts = 0.0;

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================
void readVoltage(void);
void readCurrent(void);
void printMeasurements(void);
void sendBluetoothData(void);
void printStartupHeader(void);
void checkBluetoothCommands(void);

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  // Setup Serial Monitor
  Serial.begin(9600);
  
  // Setup Bluetooth SoftwareSerial
  BTSerial. begin(BT_BAUD_RATE);
  
  // Print startup header to both Serial and Bluetooth
  printStartupHeader();
  
  // Allow sensors and Bluetooth module to stabilize
  delay(500);
}

// =============================================================================
// STARTUP HEADER FUNCTION
// Prints configuration info to Serial Monitor and Bluetooth
// =============================================================================
void printStartupHeader(void) {
  // Header strings (using F() macro to save RAM)
  Serial.println(F("=============================================="));
  Serial.println(F("  DC Voltage & Current Monitor v2.1"));
  Serial.println(F("  Bluetooth Edition"));
  Serial.println(F("=============================================="));
  Serial.println();
  Serial.println(F("CALIBRATION PARAMETERS:"));
  Serial.println(F("----------------------------------------------"));
  Serial.print(F("  Zero-Current ADC Offset:   "));
  Serial.print(ZERO_CURRENT_ADC_OFFSET);
  Serial.println(F(" (raw ADC value)"));
  Serial.print(F("  Zero-Point Voltage:       "));
  Serial.print((ZERO_CURRENT_ADC_OFFSET * ref_voltage) / ADC_RESOLUTION, 3);
  Serial.println(F(" V"));
  Serial.println(F("  Theoretical Zero-Point:   2.500 V (ADC 512)"));
  Serial.println();
  Serial.println(F("HARDWARE CONFIGURATION:"));
  Serial.println(F("----------------------------------------------"));
  Serial.print(F("  Voltage Divider:  R1="));
  Serial.print(R1/1000, 1);
  Serial.print(F("k, R2="));
  Serial.print(R2/1000, 1);
  Serial.println(F("k"));
  Serial.print(F("  Max Input Voltage:  "));
  Serial.print(ref_voltage / (R2/(R1+R2)), 1);
  Serial.println(F(" V"));
  Serial.print(F("  ACS712 Scale Factor: "));
  Serial.print(SCALE_FACTOR * 1000, 0);
  Serial.println(F(" mV/A"));
  Serial.println();
  Serial.println(F("BLUETOOTH CONFIGURATION:"));
  Serial.println(F("----------------------------------------------"));
  Serial.print(F("  RX Pin (Arduino): D"));
  Serial.println(BT_RX_PIN);
  Serial.print(F("  TX Pin (Arduino): D"));
  Serial.println(BT_TX_PIN);
  Serial.print(F("  Baud Rate: "));
  Serial.println(BT_BAUD_RATE);
  Serial.println(F("=============================================="));
  Serial.println();
  Serial.println(F("Starting measurements... "));
  Serial.println();
  
  // Send startup message to Bluetooth
  BTSerial.println(F("DC Monitor v2.1 BT Ready"));
  BTSerial.println(F("Format: V,I,P"));
  BTSerial.println(F("Commands: V I P A H C"));
}

// =============================================================================
// MAIN LOOP
// =============================================================================
void loop() {
  // Read voltage
  readVoltage();
  
  // Read current
  readCurrent();
  
  // Calculate power (P = V × I)
  powerWatts = in_voltage * abs(currentAmps);
  
  // Print all measurements to Serial Monitor
  printMeasurements();
  
  // Send data to Bluetooth
  sendBluetoothData();
  
  // Check for incoming Bluetooth commands
  checkBluetoothCommands();
  
  // Short delay before next reading
  delay(500);
}

// =============================================================================
// VOLTAGE READING FUNCTION
// Simple voltage divider calculation as per user's original code
// =============================================================================
void readVoltage(void) {
  // Read the Analog Input
  adc_value = analogRead(VOLTAGE_PIN);
  
  // Determine voltage at ADC input
  adc_voltage = (adc_value * ref_voltage) / 1024.0;
  
  // Calculate voltage at divider input
  in_voltage = adc_voltage / (R2 / (R1 + R2));
  
  // Clamp small noise to zero
  if (in_voltage < 0.09) {
    in_voltage = 0.0;
  }
}

// =============================================================================
// CURRENT READING FUNCTION
// Reads current from ACS712 sensor with calibrated offset (ADC 441)
// Uses 1000 samples for precision averaging
// =============================================================================
void readCurrent(void) {
  float voutSum = 0.0;
  
  // Take many samples for precision (ACS712 output can be noisy)
  for (int i = 0; i < CURRENT_SAMPLES; i++) {
    voutSum += ((ref_voltage / ADC_RESOLUTION) * analogRead(CURRENT_PIN));
    delay(SAMPLE_DELAY_MS);
  }
  
  // Calculate average sensor voltage
  sensorVoltage = voutSum / CURRENT_SAMPLES;
  
  // Calculate zero-point voltage from calibrated ADC offset
  float zeroPointVoltage = (ZERO_CURRENT_ADC_OFFSET * ref_voltage) / ADC_RESOLUTION;
  
  // Convert voltage to current using scale factor
  // Subtract calibrated zero-point voltage from sensor reading
  currentAmps = (sensorVoltage - zeroPointVoltage) / SCALE_FACTOR;
  
  // Apply dead-band for noise (±0.05A)
  if (abs(currentAmps) < 0.05) {
    currentAmps = 0.0;
  }
}

// =============================================================================
// SERIAL OUTPUT FUNCTION
// Prints all measurements to Serial Monitor
// =============================================================================
void printMeasurements(void) {
  // Print voltage
  Serial.print(F("Input Voltage = "));
  Serial.print(in_voltage, 2);
  Serial.print(F(" V"));
  
  // Print current
  Serial.print(F("\t Current = "));
  Serial.print(currentAmps, 2);
  Serial.print(F(" A"));
  
  // Print power
  Serial.print(F("\t Power = "));
  Serial.print(powerWatts, 2);
  Serial.println(F(" W"));
}

// =============================================================================
// BLUETOOTH OUTPUT FUNCTION
// Sends measurement data to Bluetooth device
// Format: CSV for easy parsing in mobile apps
// =============================================================================
void sendBluetoothData(void) {
  // CSV format: voltage,current,power (easy to parse in apps)
  BTSerial.print(in_voltage, 2);
  BTSerial.print(F(","));
  BTSerial.print(currentAmps, 2);
  BTSerial.print(F(","));
  BTSerial. println(powerWatts, 2);
}

// =============================================================================
// BLUETOOTH COMMAND HANDLER
// Receives and processes commands from Bluetooth device
// =============================================================================
void checkBluetoothCommands(void) {
  // Check if data is available from Bluetooth
  if (BTSerial.available()) {
    char command = BTSerial.read();
    
    // Process command
    switch (command) {
      case 'V':   // Request voltage only
      case 'v':
        BTSerial.print(F("Voltage: "));
        BTSerial.print(in_voltage, 2);
        BTSerial.println(F(" V"));
        break;
        
      case 'I':  // Request current only
      case 'i': 
        BTSerial.print(F("Current: "));
        BTSerial. print(currentAmps, 2);
        BTSerial. println(F(" A"));
        break;
        
      case 'P':  // Request power only
      case 'p':
        BTSerial.print(F("Power: "));
        BTSerial.print(powerWatts, 2);
        BTSerial.println(F(" W"));
        break;
        
      case 'A':  // Request all data (human-readable)
      case 'a': 
        BTSerial. print(F("V="));
        BTSerial.print(in_voltage, 2);
        BTSerial.print(F("V I="));
        BTSerial.print(currentAmps, 2);
        BTSerial.print(F("A P="));
        BTSerial.print(powerWatts, 2);
        BTSerial. println(F("W"));
        break;
        
      case 'H':   // Help command
      case 'h':
      case '?':
        BTSerial.println(F("=== COMMANDS ==="));
        BTSerial.println(F("V - Voltage"));
        BTSerial.println(F("I - Current"));
        BTSerial. println(F("P - Power"));
        BTSerial.println(F("A - All data"));
        BTSerial.println(F("C - Calibration"));
        BTSerial.println(F("H - Help"));
        break;
        
      case 'C':   // Calibration info
      case 'c':
        BTSerial.print(F("Zero ADC:  "));
        BTSerial.println(ZERO_CURRENT_ADC_OFFSET);
        BTSerial.print(F("Scale:  "));
        BTSerial.print(SCALE_FACTOR * 1000, 0);
        BTSerial.println(F(" mV/A"));
        break;
        
      default: 
        // Ignore unknown commands or newlines
        break;
    }
  }
}