#include <Arduino.h>

// ---------------------------
// Pin Definitions
// ---------------------------
#define ADXL335_X_PIN 34
#define ADXL335_Y_PIN 35
#define ADXL335_Z_PIN 32
#define VIBRATION_PIN 33

// ---------------------------
// Calibration and Constants
// ---------------------------
// Assuming 3.3V supply and ~300 mV/g sensitivity for ADXL335
// Adjust as necessary
const float ZERO_g_VOLTAGE = 1.65; // baseline at 0 g for ADXL335
const float SENSITIVITY = 0.3;     // V/g (approximate)
const float ADC_REF = 3.3;         // ADC reference voltage on ESP32
const int ADC_MAX = 4095;          // 12-bit ADC

// Accident detection thresholds
// Adjust these according to your scenario and experimentation
float accelerationThreshold_g = 2.5;   // e.g., >2.5g considered potential accident
float vibrationThreshold = 1.0;        // e.g., >1.0V from vibration sensor indicates strong impact

// Function to convert ADC reading to voltage
float adcToVoltage(int adcValue) {
  return ((float)adcValue / (float)ADC_MAX) * ADC_REF;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(ADXL335_X_PIN, INPUT);
  pinMode(ADXL335_Y_PIN, INPUT);
  pinMode(ADXL335_Z_PIN, INPUT);
  pinMode(VIBRATION_PIN, INPUT);

  Serial.println("Accident Detection System Started...");
}

void loop() {
  // ---------------------------
  // Read accelerometer (ADXL335)
  // ---------------------------
  int xRaw = analogRead(ADXL335_X_PIN);
  int yRaw = analogRead(ADXL335_Y_PIN);
  int zRaw = analogRead(ADXL335_Z_PIN);

  float xVolts = adcToVoltage(xRaw);
  float yVolts = adcToVoltage(yRaw);
  float zVolts = adcToVoltage(zRaw);

  // Convert to g-values
  float xG = (xVolts - ZERO_g_VOLTAGE) / SENSITIVITY;
  float yG = (yVolts - ZERO_g_VOLTAGE) / SENSITIVITY;
  float zG = (zVolts - ZERO_g_VOLTAGE) / SENSITIVITY;

  // Compute magnitude of acceleration vector
  float aMag = sqrt(xG * xG + yG * yG + zG * zG);

  // ---------------------------
  // Read Vibration Sensor
  // ---------------------------
  int vibrationRaw = analogRead(VIBRATION_PIN);
  float vibrationVolts = adcToVoltage(vibrationRaw);

  // ---------------------------
  // Accident Detection Logic
  // ---------------------------
  bool accidentDetected = false;

  // Check if acceleration exceeds threshold
  if (aMag > accelerationThreshold_g) {
    accidentDetected = true;
    Serial.println("ALERT: High acceleration detected!");
  }

  // Check vibration sensor
  if (vibrationVolts > vibrationThreshold) {
    accidentDetected = true;
    Serial.println("ALERT: Strong vibration detected!");
  }

  // If an accident is detected, take action
  if (accidentDetected) {
    Serial.println("***** POTENTIAL ACCIDENT DETECTED *****");
    // Here, you could trigger more actions:
    // - Send a message via a connected modem
    // - Log the event
    // - Light up an LED or activate a buzzer
    // For demonstration, we’ll just print to Serial.
  }

  // ---------------------------
  // Print Status
  // ---------------------------
  Serial.print("X_g: "); Serial.print(xG, 2);
  Serial.print(" | Y_g: "); Serial.print(yG, 2);
  Serial.print(" | Z_g: "); Serial.print(zG, 2);
  Serial.print(" | A_mag: "); Serial.print(aMag, 2);
  Serial.print(" g | Vib: "); Serial.print(vibrationVolts, 2); Serial.println(" V");

  delay(500);
}
