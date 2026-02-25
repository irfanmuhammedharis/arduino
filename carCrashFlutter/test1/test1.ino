/********** Libraries **********/
#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <TinyGPSPlus.h>

// Provide the token generation process info
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions
#include <addons/RTDBHelper.h>

/********** Wi-Fi & Firebase Credentials **********/
#define WIFI_SSID        "YOUR_WIFI_SSID"
#define WIFI_PASSWORD    "YOUR_WIFI_PASSWORD"
#define API_KEY          "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL     "YOUR_FIREBASE_DATABASE_URL"  // e.g. "myproject.firebaseio.com"

/********** Firebase Objects **********/
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

/********** Pin Definitions **********/
#define ADXL335_X_PIN 34
#define ADXL335_Y_PIN 35
#define ADXL335_Z_PIN 32
#define VIBRATION_PIN 33

// GPS pins (adjust if wiring differs)
#define GPS_RX_PIN 16  // GPS TX -> ESP32 RX pin
#define GPS_TX_PIN 17  // GPS RX -> ESP32 TX pin

/********** Calibration and Constants **********/
const float ZERO_g_VOLTAGE = 1.65;   // baseline at 0 g for ADXL335
const float SENSITIVITY     = 0.3;   // V/g (typical for ADXL335)
const float ADC_REF         = 3.3;   // ADC reference voltage on ESP32
const int   ADC_MAX         = 4095;  // 12-bit ADC

/********** Accident Detection Thresholds **********/
float accelerationThreshold_g = 2.5; // e.g., >2.5g -> potential accident
float vibrationThreshold      = 1.0; // e.g., >1.0V -> strong impact

/********** GPS Object **********/
TinyGPSPlus gps;

/********** Hardware Serial for GPS **********/
HardwareSerial SerialGPS(2);

/********** Helper: Convert ADC reading to voltage **********/
float adcToVoltage(int adcValue) {
  return ((float)adcValue / (float)ADC_MAX) * ADC_REF;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize sensor pins
  pinMode(ADXL335_X_PIN, INPUT);
  pinMode(ADXL335_Y_PIN, INPUT);
  pinMode(ADXL335_Z_PIN, INPUT);
  pinMode(VIBRATION_PIN,   INPUT);

  // Initialize GPS Serial
  SerialGPS.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  Serial.println("Accident Detection + GPS (Only GPS to Firebase) Starting...");

  /********** Wi-Fi Connection **********/
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  /********** Firebase Initialization **********/
  Serial.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  
  config.api_key     = API_KEY;
  config.database_url = DATABASE_URL;

  // Initialize the Firebase connection
  Firebase.begin(DATABASE_URL, API_KEY);
  
  // Set decimal places for float/double
  Firebase.setDoubleDigits(5);

  Serial.println("Firebase initialized successfully.");
}

void loop() {
  /********** 1. Read GPS Data **********/
  while (SerialGPS.available() > 0) {
    char c = SerialGPS.read();
    gps.encode(c);
  }

  /********** 2. Read ADXL335 Accelerometer **********/
  int xRaw = analogRead(ADXL335_X_PIN);
  int yRaw = analogRead(ADXL335_Y_PIN);
  int zRaw = analogRead(ADXL335_Z_PIN);

  float xVolts = adcToVoltage(xRaw);
  float yVolts = adcToVoltage(yRaw);
  float zVolts = adcToVoltage(zRaw);

  float xG = (xVolts - ZERO_g_VOLTAGE) / SENSITIVITY;
  float yG = (yVolts - ZERO_g_VOLTAGE) / SENSITIVITY;
  float zG = (zVolts - ZERO_g_VOLTAGE) / SENSITIVITY;
  
  // Calculate total acceleration magnitude
  float aMag = sqrt(xG * xG + yG * yG + zG * zG);

  /********** 3. Read Vibration Sensor **********/
  int   vibrationRaw   = analogRead(VIBRATION_PIN);
  float vibrationVolts = adcToVoltage(vibrationRaw);

  /********** 4. Accident Detection **********/
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

  /********** 5. Print Status to Serial **********/
  Serial.print("X_g: ");  Serial.print(xG, 2);
  Serial.print(" | Y_g: ");  Serial.print(yG, 2);
  Serial.print(" | Z_g: ");  Serial.print(zG, 2);
  Serial.print(" | A_mag: ");  Serial.print(aMag, 2);
  Serial.print(" g | Vib: ");  Serial.print(vibrationVolts, 2);  Serial.print(" V");

  if (gps.location.isValid()) {
    Serial.print(" | Lat: "); Serial.print(gps.location.lat(), 6);
    Serial.print(" | Lon: "); Serial.print(gps.location.lng(), 6);
  } else {
    Serial.print(" | GPS: No valid fix");
  }
  Serial.println();

  /********** 6. Send Only GPS Data if Accident Detected **********/
  if (accidentDetected && Firebase.ready()) {
    Serial.println("***** POTENTIAL ACCIDENT DETECTED *****");
    
    // Check if GPS location is valid
    if (gps.location.isValid()) {
      float latitude  = gps.location.lat();
      float longitude = gps.location.lng();

      // Print to Serial
      Serial.print("GPS Location: ");
      Serial.print(latitude, 6);
      Serial.print(", ");
      Serial.println(longitude, 6);

      // Update Firebase with only GPS location
      Firebase.setFloat(fbdo, "/accident/latitude",  latitude);
      Firebase.setFloat(fbdo, "/accident/longitude", longitude);

    } else {
      Serial.println("GPS location not available or invalid.");
      Firebase.setString(fbdo, "/accident/GPS_Status", "Invalid fix");
    }
  }

  // Small delay for loop pacing
  delay(500);
}
