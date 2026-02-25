/*************************************************************
   Firebase + Servo Control (ESP32)
   Replacing Serial commands with Firebase-based commands
*************************************************************/

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <ESP32Servo.h>

// Include Firebase Helper Add-Ons
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// WiFi credentials
#define WIFI_SSID       "Your_WiFi_SSID"
#define WIFI_PASSWORD   "Your_WiFi_Password"

// Firebase credentials
#define API_KEY         "Your_Firebase_API_Key"
#define DATABASE_URL    "https://your-project-id.firebaseio.com/"

// Create Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Flags and timing
bool signupOK = false;
unsigned long prevFirebaseCheck = 0; 
unsigned long firebaseInterval  = 2000; // Check every 2s

// Servo objects
Servo kneeServo;
Servo palmServo;
Servo fingerServo;

// Servo pins
const int kneePin   = 18;
const int palmPin   = 19;
const int fingerPin = 21;

// Helper function to move a servo smoothly
void moveServoSmooth(Servo &servo, int startAngle, int endAngle, int stepDelay) {
  if (startAngle < endAngle) {
    for (int angle = startAngle; angle <= endAngle; angle++) {
      servo.write(angle);
      delay(stepDelay);
    }
  } else {
    for (int angle = startAngle; angle >= endAngle; angle--) {
      servo.write(angle);
      delay(stepDelay);
    }
  }
}

// Move knee servo slowly from 0 to 90 and back
void moveKneeSlowly() {
  Serial.println("Knee: moving from 0 to 90...");
  moveServoSmooth(kneeServo, 0, 90, 15);
  delay(500);

  Serial.println("Knee: moving from 90 back to 0...");
  moveServoSmooth(kneeServo, 90, 0, 15);
  delay(500);
}

// Move finger servo slowly from 0 to 180 and back
void moveFingerSlowly() {
  Serial.println("Finger: moving from 0 to 180...");
  moveServoSmooth(fingerServo, 0, 180, 15);
  delay(500);

  Serial.println("Finger: moving from 180 back to 0...");
  moveServoSmooth(fingerServo, 180, 0, 15);
  delay(500);
}

// Move palm servo slowly from 45 to 90, 0, and back to 45
void movePalmSlowly() {
  Serial.println("Palm: moving from 45 to 90...");
  moveServoSmooth(palmServo, 45, 90, 15);
  delay(500);

  Serial.println("Palm: moving from 90 down to 0...");
  moveServoSmooth(palmServo, 90, 0, 15);
  delay(500);

  Serial.println("Palm: moving from 0 to 45...");
  moveServoSmooth(palmServo, 0, 45, 15);
  delay(500);
}

// Perform all movements slowly
void performAllActions() {
  Serial.println("Performing all slow actions in sequence...");
  moveKneeSlowly();
  moveFingerSlowly();
  movePalmSlowly();
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Firebase config
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up anonymously
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase signUp OK");
    signupOK = true;
  } else {
    Serial.printf("Firebase signUp Failed: %s\n", 
                  config.signer.signupError.message.c_str());
  }

  // Set token status callback function
  config.token_status_callback = tokenStatusCallback;
  
  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Attach each servo
  kneeServo.attach(kneePin);
  palmServo.attach(palmPin);
  fingerServo.attach(fingerPin);

  // Set initial positions
  kneeServo.write(0);
  palmServo.write(45);
  fingerServo.write(0);

  Serial.println("Setup complete. Awaiting Firebase commands (A, B, C, D)...");
}

void loop() {
  // Check Firebase every few seconds
  if (Firebase.ready() && signupOK && 
      (millis() - prevFirebaseCheck >= firebaseInterval || prevFirebaseCheck == 0)) {

    prevFirebaseCheck = millis();

    // Example: Read a string command from path "ServoCommand"
    if (Firebase.RTDB.getString(&fbdo, "ServoCommand")) {
      String command = fbdo.stringData();
      Serial.print("Fetched command from Firebase: ");
      Serial.println(command);

      // Compare and trigger actions
      if (command == "A") {
        moveKneeSlowly();
      }
      else if (command == "B") {
        moveFingerSlowly();
      }
      else if (command == "C") {
        movePalmSlowly();
      }
      else if (command == "D") {
        // Perform all actions 3 times
        for (int i = 0; i < 3; i++) {
          performAllActions();
        }
      }
      else {
        Serial.println("Invalid command! Use A, B, C, or D in Firebase.");
      }
    } 
    else {
      Serial.print("Failed to fetch command: ");
      Serial.println(fbdo.errorReason());
    }
  }
}
