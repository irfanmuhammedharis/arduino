#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// ThingSpeak settings
const char* ssid = "Qwerty";          // Your Wi-Fi SSID
const char* password = "12345678";  // Your Wi-Fi Password
String apiKey = "BY35PO1YR1H2CFTS"; // ThingSpeak API key
const char* server = "http://api.thingspeak.com/update";

// RFID pins and variables
#define SS_PIN 5
#define RST_PIN 4
MFRC522 rfid(SS_PIN, RST_PIN);

// Relay, Button, IR sensor, and Current Sensor pins
#define RELAY_PIN 15
#define BUTTON_PIN 14         // Button pin to turn off relay
#define IR_SENSOR_PIN 13      // IR sensor pin
#define CURRENT_SENSOR_PIN 34 // Analog pin for current sensor

// Authorized RFID tag UID (replace with your tag's UID)
byte authorizedUID[] = {0xDE, 0xAD, 0xBE, 0xEF};  // Example UID

void setup() {
  // Start serial communication
  Serial.begin(115200);

  // Initialize SPI and RFID
  SPI.begin(18, 19, 23);  // SCK, MISO, MOSI for ESP32
  rfid.PCD_Init();

  // Initialize relay pin as output
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Start with relay off

  // Initialize button pin as input with internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize IR sensor pin as input
  pinMode(IR_SENSOR_PIN, INPUT);

  // Initialize Wi-Fi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");

  Serial.println("Place your RFID card near the reader...");
}

void loop() {
  // Check if a new card is present
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Print UID of the card
  Serial.print("Tag UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Check if the read UID matches the authorized UID
  if (isAuthorized(rfid.uid.uidByte, rfid.uid.size)) {
    Serial.println("Authorized card detected.");

    // Check if IR sensor is HIGH
    if (digitalRead(IR_SENSOR_PIN) == HIGH) {
      Serial.println("IR sensor detected presence. Turning ON relay.");
      digitalWrite(RELAY_PIN, HIGH);  // Turn on relay
      delay(5000);                    // Keep relay on for 5 seconds
      digitalWrite(RELAY_PIN, LOW);   // Turn off relay

      // Read current sensor value and send to ThingSpeak
      float current = readCurrentSensor();
      sendToThingSpeak(current);
    } else {
      Serial.println("IR sensor not detecting presence. Relay will not turn ON.");
    }
  } else {
    Serial.println("Unauthorized card.");
  }

  // Halt PICC
  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

  // Check if the button is pressed to turn off the relay
  if (digitalRead(BUTTON_PIN) == LOW) {  // Button is pressed (active low)
    Serial.println("Button pressed. Turning OFF relay.");
    digitalWrite(RELAY_PIN, LOW);  // Turn off relay
    delay(1000);                   // Debounce delay
  }
}

// Function to check if the read UID is authorized
bool isAuthorized(byte* uid, byte uidSize) {
  if (uidSize != sizeof(authorizedUID)) {
    return false;
  }

  for (byte i = 0; i < uidSize; i++) {
    if (uid[i] != authorizedUID[i]) {
      return false;
    }
  }

  return true;
}

// Function to read current sensor value (ACS712 example)
float readCurrentSensor() {
  int analogValue = analogRead(CURRENT_SENSOR_PIN);  // Read raw analog value
  float voltage = (analogValue / 4095.0) * 3.3;      // Convert to voltage (3.3V for ESP32)
  float current = (voltage - 2.5) / 0.185;           // Convert voltage to current (ACS712-5A sensitivity = 185mV/A)
  Serial.print("Current: ");
  Serial.print(current);
  Serial.println(" A");
  return current;
}

// Function to send current data to ThingSpeak
void sendToThingSpeak(float current) {
  if (WiFi.status() == WL_CONNECTED) {  // Check Wi-Fi connection
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey + "&field1=" + String(current);
    
    http.begin(url);           // Initialize HTTP request
    int httpResponseCode = http.GET();  // Send GET request

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    http.end();  // End HTTP connection
  } else {
    Serial.println("Wi-Fi not connected");
  }
}
