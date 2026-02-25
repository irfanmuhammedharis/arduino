#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"

// Firebase credentials
// Removed "https://" prefix from the URL to ensure proper formatting
#define FIREBASE_API_KEY "AIzaSyBnckx6IB_zK6m9i7m7gqHa4kFtv1OIQFE"
#define FIREBASE_URL "solar-monitor-24695-default-rtdb.asia-southeast1.firebasedatabase.app" // No protocol prefix

// WiFi credentials
#define WIFI_SSID "Project"
#define WIFI_PASSWORD "12345678"

// Provide the token generation process info
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions
#include "addons/RTDBHelper.h"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Pin definitions
#define CURRENT_SENSOR_PIN 34    // Analog pin connected to current sensor
#define DIGITAL_PIN_1 12         // First digital pin
#define DIGITAL_PIN_2 14         // Second digital pin

// NTP Server details for timestamping
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;    // GMT+5:30 = 19800 seconds
const int daylightOffset_sec = 0;    // No DST adjustment for GMT+5:30

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Configure pins
  pinMode(CURRENT_SENSOR_PIN, INPUT);
  pinMode(DIGITAL_PIN_1, INPUT_PULLUP);
  pinMode(DIGITAL_PIN_2, INPUT_PULLUP);
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  
  // Configure time using NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Wait for time synchronization (max 10 retries)
  struct tm timeinfo;
  int retries = 0;
  while(!getLocalTime(&timeinfo) && retries < 10) {
    Serial.println("Waiting for time synchronization...");
    delay(1000);
    retries++;
  }
  if (retries == 10) {
    Serial.println("Failed to obtain time after multiple attempts");
  } else {
    Serial.println("Time synchronized successfully!");
  }
  
  // Configure Firebase
  config.api_key = FIREBASE_API_KEY;
  config.database_url = FIREBASE_URL;
  
  // Enable test mode to bypass token generation (for testing only)
  config.signer.test_mode = true;
  
  // Enable token generation callback (if not in test mode)
  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "Time-Error";
  }
  char timeStringBuff[30];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d_%H-%M-%S", &timeinfo);
  return String(timeStringBuff);
}

float readCurrentSensor() {
  // Read the analog value and convert it to current
  int rawValue = analogRead(CURRENT_SENSOR_PIN);
  
  // Example conversion for ACS712 (adjust based on your sensor)
  float voltage = rawValue * (3.3 / 4095.0);
  float current = (voltage - 1.65) / 0.185; // For ACS712 30A module
  
  return current;
}

float getValueFromPins(bool pin1, bool pin2) {
  // Map digital pin states to percentage values
  if (!pin1 && !pin2) {
    return 25.0;
  } else if (!pin1 && pin2) {
    return 50.0;
  } else if (pin1 && !pin2) {
    return 75.0;
  } else {
    return 0.0;
  }
}

void loop() {
  // Read digital pins
  bool pin1State = digitalRead(DIGITAL_PIN_1);
  bool pin2State = digitalRead(DIGITAL_PIN_2);
  
  float currentValue;
  String pinStatus = String(pin1State) + String(pin2State);
  
  // Determine the percentage value based on digital pin states
  currentValue = getValueFromPins(pin1State, pin2State);
  Serial.print("Digital pins are ");
  Serial.print(pinStatus);
  Serial.print(", sending ");
  Serial.print(currentValue);
  Serial.println("%");
  
  // Read the current sensor
  float actualCurrent = readCurrentSensor();
  Serial.print("Actual current sensor value: ");
  Serial.println(actualCurrent);
  
  // Check if sensor value is valid (not NaN)
  if (isnan(actualCurrent)) {
    Serial.println("Sensor reading is invalid (NaN). Skipping Firebase update.");
    delay(5000);
    return;
  }
  
  // Get a timestamp for the data point
  String timestamp = getTimestamp();
  
  // Create a unique path for each data point (ensure no illegal characters are in the path)
  String path = "/current_readings/" + timestamp;
  
  // Debug: Print the full path to ensure it's correctly formed.
  Serial.print("Firebase Path: ");
  Serial.println(path);
  
  // Send data to Firebase
  if (Firebase.RTDB.setFloat(&fbdo, path + "/percentage", currentValue)) {
    Serial.println("Data sent to Firebase successfully");
    
    // Add digital pins state
    Firebase.RTDB.setString(&fbdo, path + "/pin_state", pinStatus);
    
    // Add the actual current reading
    Firebase.RTDB.setFloat(&fbdo, path + "/actual_current", actualCurrent);
    
    // Add timestamp in readable format
    Firebase.RTDB.setString(&fbdo, path + "/timestamp", timestamp);
  } else {
    Serial.println("Failed to send data to Firebase");
    Serial.println("Reason: " + fbdo.errorReason());
  }
  
  // Wait before the next reading
  delay(5000);  // Adjust the delay as needed
}
