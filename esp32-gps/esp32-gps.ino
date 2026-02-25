#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

// WiFi Credentials
const char* ssid = "Millionaire";
const char* password = "123456789";

// Telegram Bot Details
const char* BOT_TOKEN = "7138530040:AAF3guRfcCpIhXxIt9I7Rq-mw9ZuRtXyfrU";
const char* CHAT_ID = "395500469"; // Replace with your Telegram chat ID

// GPS and Button Settings
#define RXD2 16 // GPS RX Pin
#define TXD2 17 // GPS TX Pin
#define BUTTON_PIN 4 // Button GPIO pin

TinyGPSPlus gps;
HardwareSerial GPS_Serial(2);
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Global variables
bool buttonPressed = false;

// Connect to WiFi
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
}

// Send GPS location via Telegram
void sendTelegramMessage(double latitude, double longitude) {
  String message = "📍 GPS Location:\n";
  message += "Latitude: " + String(latitude, 6) + "\n";
  message += "Longitude: " + String(longitude, 6) + "\n";
  message += "Google Maps: https://www.google.com/maps?q=" + String(latitude, 6) + "," + String(longitude, 6);
  bot.sendMessage(CHAT_ID, message, "");
}

void setup() {
  Serial.begin(115200);
  GPS_Serial.begin(9600, SERIAL_8N1, RXD2, TXD2); // Initialize GPS serial communication
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);

  connectToWiFi();

  client.setCACert(nullptr); // Optionally set CA Certificate
  Serial.println("Setup complete.");
}

void loop() {
  // Read GPS data
  while (GPS_Serial.available() > 0) {
    char c = GPS_Serial.read();
    gps.encode(c);
  }

  // Check button press
  if (digitalRead(BUTTON_PIN) == HIGH && !buttonPressed) {
    buttonPressed = true;
    Serial.println("Button Pressed!");

    // Check if GPS has a valid location
    if (gps.location.isValid()) {
      double latitude = gps.location.lat();
      double longitude = gps.location.lng();
      sendTelegramMessage(latitude, longitude);
    } else {
      Serial.println("No valid GPS location available.");
      bot.sendMessage(CHAT_ID, "⚠️ Unable to get GPS location. Please try again.", "");
    }
  }

  // Reset button state when released
  if (digitalRead(BUTTON_PIN) == LOW) {
    buttonPressed = false;
  }

  delay(100); // Small delay to debounce button
}
