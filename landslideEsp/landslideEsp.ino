#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// WiFi & Telegram Credentials 🌐🤖
const char* ssid = "Hexcodeplus 4g";
const char* password = "Hexcode1234";
const String botToken = "7541066997:AAH6Kv5dPbWU0pkLTRvS10vZAvyl-zHu1CU";
const String chatID = "@lanslideproject";

// Pin assignments for sensors 📌
#define SOIL_SENSOR_PIN 35      // Soil Moisture Sensor analog input
#define VIBRATION_SENSOR_PIN 36 // Vibration Sensor analog input

// Digital input for landslide detection 🌄
#define LANDSLIDE_PIN 2         // Using D2 as the digital input

// Sensor Thresholds 💧🔔
int soilThreshold = 100;       // Adjust based on sensor calibration (soil moisture)
int vibrationThreshold = 300;  // Adjust based on sensor calibration (vibration)

// Alert State Flags (to avoid repeated alerts) 🚦
bool moistureAlertActive = false;
bool vibrationAlertActive = false;
bool landslideAlertActive = false;

// Send message to Telegram via Bot API 📲
void sendTelegramMessage(String message) {
  WiFiClientSecure client;
  client.setInsecure(); // For demo purposes only
  HTTPClient https;
  String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;
  
  if (https.begin(client, url)) {
    int httpCode = https.GET();
    if (httpCode > 0) {
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
  
  // Set LANDSLIDE_PIN as input
  pinMode(LANDSLIDE_PIN, INPUT);
  
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
  Serial.println("Starting Sensor & Telegram Monitoring... 😊");
}

void loop() {
  // ----- Check Land Slide via D2 -----
  int landslideState = digitalRead(LANDSLIDE_PIN);
  if (landslideState == HIGH) {
    if (!landslideAlertActive) {
      String message = "Alert: land slide detected ⚠️";
       sendTelegramMessage(message);
      Serial.println(message);
      landslideAlertActive = true;
    }
  } else {
    landslideAlertActive = false;
  }
  
  // ----- Soil Moisture Measurement -----
  int soilValue = analogRead(SOIL_SENSOR_PIN);
  Serial.print("Soil moisture reading: ");
  Serial.println(soilValue);
  
  if (soilValue < soilThreshold) {
    if (!moistureAlertActive) {
      String message = "Alert: Soil is too wet! (Moisture reading: " + String(soilValue) + ") 💧";
       sendTelegramMessage(message);
      Serial.println(message);
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
       sendTelegramMessage(message);
      Serial.println(message);
      vibrationAlertActive = true;
    }
  } else {
    vibrationAlertActive = false;
  }
  
  delay(500); // Delay before next reading ⏲️
}
