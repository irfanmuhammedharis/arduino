#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <math.h>

// WiFi & Telegram Credentials 🌐🤖
const char* ssid = "Hexcodeplus 4g";
const char* password = "Hexcode1234";
const String botToken = "7541066997:AAH6Kv5dPbWU0pkLTRvS10vZAvyl-zHu1CU";
const String chatID = "@lanslideproject";

// Pin assignments for sensors 📌
// ADXL335 Accelerometer Pins (ADC1 channels)
#define X_PIN 32   // X-axis analog input
#define Y_PIN 33   // Y-axis analog input
#define Z_PIN 34   // Z-axis analog input
// Other Sensors
#define SOIL_SENSOR_PIN 35      // Soil Moisture Sensor analog input
// Vibration sensor reassigned to GPIO27 (an ADC2 channel) due to board pin limitations
#define VIBRATION_SENSOR_PIN 36 // Vibration Sensor analog input

// ADXL335 Calibration Parameters 📏
float zeroGVoltage = 1.65;    // Typical 0g voltage for a 3.3V powered ADXL335
float sensitivity = 0.3;      // ADXL335 sensitivity: 300 mV/g
float voltageReference = 3.3;
int adcResolution = 4095;     // 12-bit ADC resolution

// Sensor Thresholds 💧🔔
int soilThreshold = 100;       // Adjust based on sensor calibration (soil moisture)
int vibrationThreshold = 300;  // Adjust based on sensor calibration (vibration)
float tiltAlertThreshold = 5.0; // 5° threshold for tilt alerts

// Alert State Flags (to avoid repeated alerts) 🚦
bool tiltAlertActive = false;
bool moistureAlertActive = false;
bool vibrationAlertActive = false;

// Convert ADC value to voltage 🔄
float getVoltage(int adcValue) {
  return ((float)adcValue / adcResolution) * voltageReference;
}

// Send message to Telegram via Bot API 📲
void sendTelegramMessage(String message) {
  WiFiClientSecure client;
  client.setInsecure(); // For demo purposes only! Use proper certificate validation in production.
  
  HTTPClient https;
  String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;
  
  if (https.begin(client, url)) {
    int httpCode = https.GET();
    if(httpCode > 0) {
      Serial.print("Telegram Message sent, HTTP code: ");
      Serial.println(httpCode);
    } else {
      Serial.print("Error sending Telegram message: ");
      Serial.println(https.errorToString(httpCode));
    }
    https.end();
  } else {
    Serial.println("Unable to connect to Telegram API");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi 🚀
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected! IP address: " + WiFi.localIP().toString());
  
  // Allow sensors to stabilize ⏳
  delay(1000);
  Serial.println("Starting Multi-Sensor & Telegram Monitoring... 😊");
}

void loop() {
  // ----- ADXL335 Tilt Measurement using X, Y, Z axes -----
  int adcX = analogRead(X_PIN);
  int adcY = analogRead(Y_PIN);
  int adcZ = analogRead(Z_PIN);
  
  float voltageX = getVoltage(adcX);
  float voltageY = getVoltage(adcY);
  float voltageZ = getVoltage(adcZ);
  
  // Calculate acceleration in g's for each axis
  float ax = (voltageX - zeroGVoltage) / sensitivity;
  float ay = (voltageY - zeroGVoltage) / sensitivity;
  float az = (voltageZ - zeroGVoltage) / sensitivity;
  
  // Compute pitch, roll, and overall tilt
  float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
  float roll  = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  float norm  = sqrt(ax * ax + ay * ay + az * az);
  float overallTilt = acos(az / norm) * 180.0 / PI;
  
  Serial.print("Pitch: ");
  Serial.print(pitch);
  Serial.print("°, Roll: ");
  Serial.print(roll);
  Serial.print("°, Overall Tilt: ");
  Serial.print(overallTilt);
  Serial.println("°");
  
  // Tilt Alert Check 🚨
  if ((abs(pitch) > tiltAlertThreshold) || (abs(roll) > tiltAlertThreshold) || (overallTilt > tiltAlertThreshold)) {
    if (!tiltAlertActive) {
      String message = "Alert: Tilt exceeded threshold!\nPitch: " + String(pitch) + "°, Roll: " + String(roll) + "°, Overall Tilt: " + String(overallTilt) + "° ⚠️";
     // sendTelegramMessage(message);
      tiltAlertActive = true;
    }
  } else {
    tiltAlertActive = false;
  }
  
  // ----- Soil Moisture Measurement -----
  int soilValue = analogRead(SOIL_SENSOR_PIN);
  Serial.print("Soil moisture reading: ");
  Serial.println(soilValue);
  
  if (soilValue < soilThreshold) {
    if (!moistureAlertActive) {
      String message = "Alert: Soil is too wet! (Moisture reading: " + String(soilValue) + ") 💧";
     // sendTelegramMessage(message);
      moistureAlertActive = true;
    }
  } else {
    moistureAlertActive = false;
  }
  
  // ----- Vibration Sensor Measurement -----
  int vibrationValue = analogRead(VIBRATION_SENSOR_PIN);
  Serial.print("Vibration sensor reading: ");
  Serial.println(vibrationValue);
  
  if (vibrationValue > vibrationThreshold) {
    if (!vibrationAlertActive) {
      String message = "Alert: Vibration detected! (Vibration reading: " + String(vibrationValue) + ") 🔔";
     // sendTelegramMessage(message);
      vibrationAlertActive = true;
    }
  } else {
    vibrationAlertActive = false;
  }
  
  delay(500); // Delay before next reading ⏲️
}
