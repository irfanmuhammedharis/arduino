#include <ESP32Servo.h>

// Create Servo objects
Servo kneeServo;
Servo palmServo;
Servo fingerServo;

// Define the servo control pins
const int kneePin   = 18;
const int palmPin   = 19;
const int fingerPin = 21;
int i;

// Helper function to move a servo smoothly
void moveServoSmooth(Servo &servo, int startAngle, int endAngle, int stepDelay) {
  if (startAngle < endAngle) {
    // Increment from startAngle to endAngle
    for (int angle = startAngle; angle <= endAngle; angle++) {
      servo.write(angle);
      delay(stepDelay);
    }
  } else {
    // Decrement from startAngle to endAngle
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
  
  // Attach each servo to its respective pin
  kneeServo.attach(kneePin);
  palmServo.attach(palmPin);
  fingerServo.attach(fingerPin);

  // Set initial positions
  kneeServo.write(0);
  palmServo.write(45);
  fingerServo.write(0);

  Serial.println("Setup complete. Waiting for commands (A, B, C, D)...");
}

void loop() {
  // Check if data is available on serial
  if (Serial.available() > 0) {
    char command = Serial.read();

    // Debug prints
    Serial.print("Received char: '");
    Serial.print(command);
    Serial.print("'  ASCII value: ");
    Serial.println((int)command);

    // Ignore line endings
    if (command == '\n' || command == '\r') {
      Serial.println("Ignored a line ending character.");
      return;
    }

    // Execute commands
    switch (command) {
      case 'A':
        moveKneeSlowly();
        break;

      case 'B':
        moveFingerSlowly();
        break;

      case 'C':
        movePalmSlowly();
        break;

      case 'D':
      for(i=0;i<3;i++){
        performAllActions();
      };
        break;

      default:
        Serial.println("Invalid command! Use A, B, C, or D.");
        break;
    }
  }
}
