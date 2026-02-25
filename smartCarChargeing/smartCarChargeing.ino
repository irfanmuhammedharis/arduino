#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ThingSpeak.h>

#define SS_PIN 5                  // RFID SS pin for ESP32
#define RST_PIN 22                 // RFID RST pin for ESP32
#define RELAY_PIN 13              // Relay control pin for ESP32
#define CURRENT_SENSOR_PIN 34     // ADC pin for current sensor on ESP32
#define IR_SENSOR_PIN 12          // IR sensor digital output pin

// Wi-Fi credentials and ThingSpeak API key
const char* ssid = "sreelekshmi";
const char* password = "sree1234";
const char* apiKey = "GMXX6ZFMP6KWQBB6"; // ThingSpeak API Key

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
WiFiClient client;

unsigned long channelID = 2725622; // Replace with your ThingSpeak channel ID

// List of authorized RFID tags
String authorizedRFIDs[] = {
  "B5 E1 AC AC"
 // "YOUR_AUTHORIZED_RFID_TAG_2",
 // "YOUR_AUTHORIZED_RFID_TAG_3"
};
const int numAuthorizedRFIDs = sizeof(authorizedRFIDs) / sizeof(authorizedRFIDs[0]);

void setup() {
  Serial.begin(115200);
  SPI.begin();             // Initiate SPI bus
  mfrc522.PCD_Init();      // Initiate MFRC522
  pinMode(RELAY_PIN, OUTPUT); 
  pinMode(IR_SENSOR_PIN, INPUT);   // Set IR sensor pin as input
  digitalWrite(RELAY_PIN, LOW);    // Ensure relay is off

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" Connected to WiFi");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

// Function to check if an RFID is in the authorized list
bool isAuthorized(String rfid) {
  for (int i = 0; i < numAuthorizedRFIDs; i++) {
    if (rfid.equalsIgnoreCase(authorizedRFIDs[i])) {
      return true;
    }
  }
  return false;
}

void loop() {
  // Check for RFID card presence
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String rfid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      rfid += String(mfrc522.uid.uidByte[i], HEX); // Convert byte data to hex string
    }
    Serial.print("RFID Tag: ");
    Serial.println(rfid);

    // Check if the scanned RFID is in the authorized list
    if (isAuthorized(rfid)) {
      Serial.println("Authorized Tag Detected");

      // Check IR sensor state
      if (digitalRead(IR_SENSOR_PIN) == HIGH) {
        Serial.println("IR Sensor High - Object Detected");
        digitalWrite(RELAY_PIN, HIGH);   // Turn on relay

        // Read and calculate current from sensor
        int sensorValue = analogRead(CURRENT_SENSOR_PIN);
        float current = sensorValue * (3.3 / 4095.0);  // Convert to voltage, adjust based on sensor's specs
        Serial.print("Current Reading: ");
        Serial.println(current);

        // Send data to ThingSpeak
        ThingSpeak.setField(3, current); // Field 1 for current data
        int responseCode = ThingSpeak.writeFields(channelID, apiKey);
        if (responseCode == 200) {
          Serial.println("Data sent to ThingSpeak successfully");
        } else {
          Serial.print("Error sending data: ");
          Serial.println(responseCode);
        }
        delay(15000);  // ThingSpeak update rate limit

        digitalWrite(RELAY_PIN, LOW);  // Turn off relay after delay
      } else {
        Serial.println("IR Sensor Low - No Object Detected");
      }
    } else {
      Serial.println("Unauthorized Tag");
    }
    mfrc522.PICC_HaltA(); // Stop reading
  }
}
