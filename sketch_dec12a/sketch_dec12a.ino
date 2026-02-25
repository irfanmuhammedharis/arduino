#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// ==========================================
// USER CONFIGURATION (Required Credentials)
// ==========================================
#define WIFI_SSID "REPLACE_WITH_YOUR_SSID"
#define WIFI_PASSWORD "REPLACE_WITH_YOUR_PASSWORD"

#define Web_API_KEY "REPLACE_WITH_YOUR_FIREBASE_PROJECT_API_KEY"
#define DATABASE_URL "REPLACE_WITH_YOUR_FIREBASE_DATABASE_URL"
#define USER_EMAIL "REPLACE_WITH_FIREBASE_PROJECT_EMAIL_USER"
#define USER_PASS "REPLACE_WITH_FIREBASE_PROJECT_USER_PASS"

// ==========================================
// SENSOR CONFIGURATION
// ==========================================
const int irPin = 14;  // Connect IR Sensor OUT pin here (e.g., GPIO 14)

// User function
void processData(AsyncResult &aResult);

// Authentication
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

// Firebase components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// Timer variables for sending data every 10 seconds
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds in milliseconds

// Variable to hold the IR sensor reading (0 or 1)
int irSensorValue = 0; 


void setup(){
  Serial.begin(115200);

  // Initialize IR Sensor Pin
  pinMode(irPin, INPUT);
  
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected to WiFi.");
  
  // Configure SSL client
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
  
  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, " authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
}

void loop(){
  // Maintain authentication and async tasks
  app.loop();
  
  // Check if authentication is ready
  if (app.ready()){ 
    // Periodic data sending every 10 seconds
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval){
      
      // Update the last send time
      lastSendTime = currentTime;
      
      // -------------------------------------------------------
      // 1. READ IR SENSOR
      // -------------------------------------------------------
      // Read the digital state: 0 (LOW) = Obstacle Detected, 1 (HIGH) = No Obstacle
      irSensorValue = digitalRead(irPin);
      Serial.print("IR Sensor State: ");
      Serial.println(irSensorValue);
      
      // -------------------------------------------------------
      // 2. SEND TO FIREBASE
      // -------------------------------------------------------
      // Send the IR sensor value (0 or 1) to a path like /sensor_data/ir_status
      Database.set<int>(aClient, "/sensor_data/ir_status", irSensorValue, processData, "RTDB_Send_IR_Status");

      // Optional: Send a timestamp along with the reading
      Database.set<unsigned long>(aClient, "/sensor_data/timestamp", currentTime, processData, "RTDB_Send_Timestamp");

      Serial.println("Data set request sent.");

    }
  }
}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available())
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
}