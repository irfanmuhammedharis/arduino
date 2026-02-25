#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi Credentials
const char* ssid = "Millionaire";
const char* password = "123456789";

// Telegram Bot Details
const char* botToken = "7878529520:AAGiH2I3VC7BjzRSQeX3e7_Ad2gHhV7WF8U";
const char* chatID = "7314871251";

// TinyGPS++ object
TinyGPSPlus gps;

// Define RX and TX pins for Serial2
#define RX_PIN 16
#define TX_PIN 17

// Define Button Pin
#define BUTTON_PIN 14

void setup() {
  Serial.begin(115200); // For debugging
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // GPS communication
  delay(3000);

  // Configure button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi!");

  Serial.println(F("GPS Module and Telegram Bot Initialized"));
}

void loop() {
  // Read data from the GPS module
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    gps.encode(c);
  }

  // Check if the button is pressed
  if (digitalRead(BUTTON_PIN) == LOW) { // Button is active when pressed
    delay(50); // Debounce delay
    if (digitalRead(BUTTON_PIN) == LOW) { // Confirm button press
      if (gps.location.isValid()) {
        String message = createTelegramMessage();
        sendTelegramMessage(message);
      } else {
        Serial.println(F("Location not valid. Waiting for a GPS fix..."));
        sendTelegramMessage("GPS location not valid. Waiting for a fix...");
      }

      // Wait until the button is released
      while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
      }
    }
  }
}

String createTelegramMessage() {
  String message = "Geo-Location Update:\n";
  message += "Latitude: " + String(gps.location.lat(), 6) + "\n";
  message += "Longitude: " + String(gps.location.lng(), 6) + "\n";

  if (gps.date.isValid()) {
    message += "Date: " + String(gps.date.day()) + "/" +
               String(gps.date.month()) + "/" +
               String(gps.date.year()) + "\n";
  } else {
    message += "Date: INVALID\n";
  }

  if (gps.time.isValid()) {
    // Convert UTC to IST
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();

    // Add 5 hours and 30 minutes to UTC
    hour += 5;
    minute += 30;

    if (minute >= 60) {
      minute -= 60;
      hour += 1;
    }

    if (hour >= 24) {
      hour -= 24; // Adjust for the next day
    }

    message += "Time (IST): " + String(hour) + ":" +
               (minute < 10 ? "0" : "") + String(minute) + ":" +
               (second < 10 ? "0" : "") + String(second) + "\n";
  } else {
    message += "Time: INVALID\n";
  }

  return message;
}

void sendTelegramMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // Encode the message to handle spaces and special characters
    message.replace(" ", "%20");
    message.replace("\n", "%0A");

    String url = "https://api.telegram.org/bot" + String(botToken) +
                 "/sendMessage?chat_id=" + String(chatID) +
                 "&text=" + message;

    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.println(F("Message sent to Telegram successfully!"));
    } else {
      Serial.println("Error in sending message: " + String(httpResponseCode));
    }

    http.end(); // Free resources
  } else {
    Serial.println(F("Wi-Fi not connected. Unable to send message."));
  }
}
