#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

// Replace with your network credentials
const char* ssid = "Millionaire";
const char* password = "123456789";

// First Telegram BOT
#define BOTtoken1 "7367666959:AAEyZhb_rfr4V4xLR3HDTjRjnM2dNM7o96U"  // First Bot Token
#define CHAT_ID1 "1882246016" // First Chat ID

// Second Telegram BOT
#define BOTtoken2 "8083049196:AAGit9zb3TKNM-IHE3Cc2WqDDPHRjRWYe1Y"  // Second Bot Token
#define CHAT_ID2 "1882246016" // Second Chat ID

// SoftwareSerial setup (RX = D2, TX = D3)
SoftwareSerial softSerial(D5, D6);

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client1;
WiFiClientSecure client2;
UniversalTelegramBot bot1(BOTtoken1, client1);
UniversalTelegramBot bot2(BOTtoken2, client2);

void setup() {
  Serial.begin(115200);
  softSerial.begin(9600); // Set the baud rate for SoftwareSerial

  configTime(0, 0, "pool.ntp.org");      // Get UTC time via NTP
  client1.setTrustAnchors(&cert);       // Add root certificate for api.telegram.org for Bot 1
  client2.setTrustAnchors(&cert);       // Add root certificate for api.telegram.org for Bot 2

  // Attempt to connect to Wi-Fi
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  bot1.sendMessage(CHAT_ID1, "Bot 1 started up", "");
  bot2.sendMessage(CHAT_ID2, "Bot 2 started up", "");
}

void loop() {
  // Check if data is available on SoftwareSerial
  if (softSerial.available()) {
    String receivedData = softSerial.readStringUntil('\n'); // Read until newline character
    receivedData.trim(); // Remove any trailing or leading whitespace

    if (receivedData.length() > 0) { // Ensure there is data to send
      if (receivedData.startsWith("2")) { // Check if the data starts with '2'
        bot2.sendMessage(CHAT_ID2, "accident location: " + receivedData, "");
        Serial.println("Data sent to Bot 2: " + receivedData);
      } else {
        bot1.sendMessage(CHAT_ID1, "accident location: " + receivedData, "");
        Serial.println("Data sent to Bot 1: " + receivedData);
      }
    }
  }

  delay(10000); // Small delay to avoid spamming
}
