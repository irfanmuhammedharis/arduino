#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPClient.h>

#define SS_PIN 5         // SS (SDA) pin of the RFID module
#define RST_PIN 22       // RST pin of the RFID module
#define SENSOR_PIN 34    // Analog pin to which your current sensor is connected
#define IR_PIN 14        // Digital pin to which your IR sensor is connected
#define RELAY_PIN 26     // Digital pin to control the relay

// WiFi credentials
const char* ssid = "sreelekshmi";
const char* password = "sree1234";

// ThingSpeak settings
const char* server = "api.thingspeak.com";
String apiKey = "CQ4JO5DMQMKHF7TT";

// Authorized UID
byte authorizedUID[] = {0xB5, 0xE1, 0xAC, 0xAC};

// RFID instance
MFRC522 rfid(SS_PIN, RST_PIN);

bool sendingData = false;       // Track sending state
unsigned long previousMillis = 0; // Timer variable
const long interval = 1000;     // 1-second interval for integration
float totalCurrent = 0;         // Total accumulated current
bool irSensorState = LOW;       // Track last known state of the IR sensor

void setup() {
  Serial.begin(115200);
  SPI.begin();           // Initialize SPI bus
  rfid.PCD_Init();       // Initialize RFID reader
  pinMode(SENSOR_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // Initial relay state based on IR sensor
  digitalWrite(RELAY_PIN, HIGH); // Relay OFF initially

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void loop() {
  // Check the current state of the IR sensor
  bool currentIrState = digitalRead(IR_PIN);
  
  // Print status of IR sensor and control relay when its state changes
  if (currentIrState != irSensorState) {
    irSensorState = currentIrState;
    
    if (irSensorState == LOW) {
      Serial.println("IR sensor activated: Device is ready to operate.");
      //digitalWrite(RELAY_PIN, LOW);  // Turn relay OFF when IR is HIGH (object detected)
    } else {
      Serial.println("IR sensor deactivated: Device operation halted.");
      //digitalWrite(RELAY_PIN, HIGH); // Turn relay ON when IR is LOW (no object detected)
    }
  }

  // Proceed only if the IR sensor is activated
  if (irSensorState == LOW) {
    // Check if an RFID tag is present
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      // Toggle sending state if the authorized RFID tag is detected
      if (isAuthorizedUID()) {
         digitalWrite(RELAY_PIN, LOW);
        sendingData = !sendingData; // Toggle the sending state
        if (sendingData) {
          
          Serial.println("Authorized RFID tag detected - Starting current accumulation.");
          totalCurrent = 0;  // Reset total current when starting a new measurement
          previousMillis = millis();  // Reset timing for integration
        } else {
          digitalWrite(RELAY_PIN, HIGH);
          Serial.println("Authorized RFID tag detected - Stopping current accumulation.");
          sendToThingSpeak(totalCurrent);  // Send accumulated current to ThingSpeak
        }
      } else {
        Serial.println("Unauthorized RFID tag detected");
      }

      // Halt the current PICC to prevent repeated readings
      rfid.PICC_HaltA();
      delay(2000);  // Short delay before scanning again
    }

    // If sending data is active, accumulate current readings
    if (sendingData) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        // Read the current sensor value and convert to actual current if necessary
        int sensorValue = analogRead(SENSOR_PIN);
        float current = sensorValueToCurrent(sensorValue);  // Convert sensor reading to current
        totalCurrent += current;  // Accumulate total current
        
        Serial.print("Current reading: ");
        Serial.println(current);
        Serial.print("Accumulated Total Current: ");
        Serial.println(totalCurrent);
      }
    }
  } else {
    // Reset sending state and accumulated current if IR sensor goes low
    if (sendingData) {
      Serial.println("IR sensor deactivated. Resetting accumulation.");
      sendingData = false;
      totalCurrent = 0;
    }
  }
}

bool isAuthorizedUID() {
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != authorizedUID[i]) {
      return false;
    }
  }
  return true;
}

// Function to send total current to ThingSpeak
void sendToThingSpeak(float totalCurrent) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://" + String(server) + "/update?api_key=" + apiKey + "&field3=" + String(totalCurrent);

    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("Total current sent to ThingSpeak. HTTP response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error in sending data. HTTP response code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected. Could not send data.");
  }
}

// Placeholder function to convert sensor value to actual current
float sensorValueToCurrent(int sensorValue) {
  // Implement conversion based on your specific sensor calibration
  // Example: return (sensorValue / 1024.0) * maxCurrent;
  return (sensorValue / 1024.0) * 0.5; // assuming max 0.5A current for demonstration
}
