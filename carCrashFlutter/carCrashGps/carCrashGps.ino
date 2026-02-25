#include <Arduino.h>
#include <TinyGPSPlus.h>

// ---------------------------
// Pin Definitions
// ---------------------------
#define ADXL335_X_PIN 34
#define ADXL335_Y_PIN 35
#define ADXL335_Z_PIN 32
#define VIBRATION_PIN 33

// GPS pins (adjust according to your wiring)
#define GPS_RX_PIN 16  // GPS TX -> ESP32 RX pin;
#define GPS_TX_PIN 17  // GPS RX -> ESP32 TX pin

// ---------------------------
// Calibration and Constants
// ---------------------------
const float ZERO_g_VOLTAGE = 1.65; // baseline at 0 g for ADXL335
const float SENSITIVITY = 0.3;     // V/g (approximate)
const float ADC_REF = 3.3;         // ADC reference voltage on ESP32
const int ADC_MAX = 4095;          // 12-bit ADC

// Accident detection thresholds
float accelerationThreshold_g = 2.5; // e.g., >2.5g considered potential accident
float vibrationThreshold = 1.0;      // e.g., >1.0V from vibration sensor indicates strong impact

// Function to convert ADC reading to voltage
float adcToVoltage(int adcValue) {
  return ((float)adcValue / (float)ADC_MAX) * ADC_REF;
}

// TinyGPS++ object
TinyGPSPlus gps;

// Create a hardware serial instance for GPS (Using UART2)
HardwareSerial SerialGPS(2);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize pins
  pinMode(ADXL335_X_PIN, INPUT);
  pinMode(ADXL335_Y_PIN, INPUT);
  pinMode(ADXL335_Z_PIN, INPUT);
  pinMode(VIBRATION_PIN, INPUT);

  // Initialize GPS Serial
  SerialGPS.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  Serial.println("Accident Detection + GPS System Started...");
}

void loop() {
  // ---------------------------
  // Read GPS Data
  // ---------------------------
  while (SerialGPS.available() > 0) {
    char c = SerialGPS.read();
    gps.encode(c);
  }

  // ---------------------------
  // Read accelerometer (ADXL335)
  // ---------------------------
  int xRaw = analogRead(ADXL335_X_PIN);
  int yRaw = analogRead(ADXL335_Y_PIN);
  int zRaw = analogRead(ADXL335_Z_PIN);

  float xVolts = adcToVoltage(xRaw);
  float yVolts = adcToVoltage(yRaw);
  float zVolts = adcToVoltage(zRaw);

  float xG = (xVolts - ZERO_g_VOLTAGE) / SENSITIVITY;
  float yG = (yVolts - ZERO_g_VOLTAGE) / SENSITIVITY;
  float zG = (zVolts - ZERO_g_VOLTAGE) / SENSITIVITY;

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

  // Check acceleration threshold
  if (aMag > accelerationThreshold_g) {
    accidentDetected = true;
    Serial.println("ALERT: High acceleration detected!");
  }

  // Check vibration threshold
  if (vibrationVolts > vibrationThreshold) {
    accidentDetected = true;
    Serial.println("ALERT: Strong vibration detected!");
  }

  // If an accident is detected, display GPS location (if valid)
  if (accidentDetected) {
    Serial.println("***** POTENTIAL ACCIDENT DETECTED *****");

    // Check if GPS location is valid
    if (gps.location.isValid()) {
      Serial.print("GPS Location: ");
      Serial.print(gps.location.lat(), 6);
      Serial.print(", ");
      Serial.println(gps.location.lng(), 6);
    } else {
      Serial.println("GPS location not available or invalid.");
    }
  }

  // ---------------------------
  // Print Status
  // ---------------------------
  Serial.print("X_g: "); Serial.print(xG, 2);
  Serial.print(" | Y_g: "); Serial.print(yG, 2);
  Serial.print(" | Z_g: "); Serial.print(zG, 2);
  Serial.print(" | A_mag: "); Serial.print(aMag, 2);
  Serial.print(" g | Vib: "); Serial.print(vibrationVolts, 2); Serial.print(" V");

  // Print GPS status
  if (gps.location.isValid()) {
    Serial.print(" | Lat: "); Serial.print(gps.location.lat(), 6);
    Serial.print(" | Lon: "); Serial.print(gps.location.lng(), 6);
  } else {
    Serial.print(" | GPS: No valid fix");
  }

  Serial.println();
  delay(500);
}
