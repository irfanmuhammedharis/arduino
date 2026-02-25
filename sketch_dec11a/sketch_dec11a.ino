#include <WiFi.h>
#include "ThingSpeak.h"

// ==========================================
// USER CONFIGURATION
// ==========================================
const char* ssid = "REPLACE_WITH_YOUR_SSID";   // your network SSID (name) 
const char* password = "REPLACE_WITH_YOUR_PASSWORD";   // your network password
unsigned long myChannelNumber = 1;               // REPLACE X with your channel number
const char * myWriteAPIKey = "XXXXXXXXXXXXXXXX"; // REPLACE with your Write API Key

// ==========================================
// PIN DEFINITIONS
// ==========================================
const int irPin = 14;  // Connect IR Sensor OUT pin here

WiFiClient client;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000; // 30 seconds

// Variable to hold sensor reading
int irSensorValue; 

void setup() {
  Serial.begin(115200);  // Initialize serial

  // Initialize IR Sensor Pin
  pinMode(irPin, INPUT);
  
  WiFi.mode(WIFI_STA);   
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    
    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password); 
        delay(5000);     
      } 
      Serial.println("\nConnected.");
    }

    // -------------------------------------------------------
    // READ IR SENSOR
    // -------------------------------------------------------
    // Usually returns: 
    // 0 (LOW) = Obstacle Detected
    // 1 (HIGH) = No Obstacle
    irSensorValue = digitalRead(irPin);

    Serial.print("IR Sensor State: ");
    Serial.println(irSensorValue);
    
    // -------------------------------------------------------
    // SEND TO THINGSPEAK
    // -------------------------------------------------------
    // Write the IR value (0 or 1) to Field 1
    int x = ThingSpeak.writeField(myChannelNumber, 1, irSensorValue, myWriteAPIKey);

    if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    lastTime = millis();
  }
}