#include <WiFi.h>
#include <FirebaseESP32.h>

// WiFi credentials
#define WIFI_SSID "Millionaire"
#define WIFI_PASSWORD "12345678"

// Firebase credentials
#define FIREBASE_HOST "https://accident-alert-5f157-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyB-8NUEoq-LAeULXuUR_6nhaiIcfc_rcjc"

// Create Firebase and Wi-Fi client objects
FirebaseData firebaseData;

// GPS mock data structure
struct GPSData {
  float latitude;
  float longitude;
  String accidentSeverity;
} gpsData;

// Function to initialize Wi-Fi
void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to Wi-Fi");
}

// Function to send data to Firebase
void sendToFirebase(GPSData data) {
  String path = "/gps_data";
  FirebaseJson json;

  json.add("latitude", data.latitude);
  json.add("longitude", data.longitude);
  json.add("accident_severity", data.accidentSeverity);

  if (Firebase.pushJSON(firebaseData, path, json)) {
    Serial.println("Data sent successfully");
  } else {
    Serial.print("Firebase push failed: ");
    Serial.println(firebaseData.errorReason());
  }
}

// Mock GPS data generator
void generateMockGPSData() {
  gpsData.latitude = 37.7749 + (random(-100, 100) / 10000.0); // Simulated lat offset
  gpsData.longitude = -122.4194 + (random(-100, 100) / 10000.0); // Simulated long offset

  // Toggle accident severity
  if (random(0, 2) == 0) {
    gpsData.accidentSeverity = "minor";
  } else {
    gpsData.accidentSeverity = "major";
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  connectToWiFi();

  // Initialize Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  Serial.println("Setup complete");
}

void loop() {
  generateMockGPSData();
  Serial.println("Sending GPS data to Firebase...");
  sendToFirebase(gpsData);

  // Wait for 30 seconds before sending the next data
  delay(30000);
}
