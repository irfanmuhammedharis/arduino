/* 
   Motor Pins:
   in1 -> 12, in2 -> 14, in3 -> 26, in4 -> 27

   Servos:
   servo1 -> 2
   servo2 -> 21
   servo3 -> 22
   servo4 -> 23
   servo5 -> 15
*/

#include <SPI.h>
#include <ESP32Servo.h>  
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

// Define Pin Constants
#define right1 12
#define right2 14
#define left1  27
#define left2  26
#define led    5
#define TRIGGER_PIN 4
#define ECHO_PIN    13

// Speed of sound in cm/µs
#define SOUND_SPEED 0.034

// Ultrasonic Distance Threshold
#define MAX_DISTANCE 400

// Robot Command Defines
#define FORWARD  'F'
#define BACKWARD 'B'
#define LEFT     'L'
#define RIGHT    'R'
#define CIRCLE   'C'
#define CROSS    'X'
#define TRIANGLE 'T'
#define SQUARE   'S'
#define START    'A'
#define PAUSE    'P'

// Global Variables
long duration;
float distanceCm; 

// Servos
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;
Servo servo5;

int pos  = 45;
int posh = 90;
int servoAngle1 = 0;

// Forward Declarations
void executeCommand(char command);
void action();
float measureDistance();

// --------------------------------------------------
// Setup
// --------------------------------------------------
void setup() {
  SerialBT.begin("robo");  // Initialize Bluetooth
  Serial.begin(115200);    // Initialize Serial Monitor

  // Pin modes for motors & LED
  pinMode(right1, OUTPUT);
  pinMode(right2, OUTPUT);
  pinMode(left1,  OUTPUT);
  pinMode(left2,  OUTPUT);
  pinMode(led,    OUTPUT);

  // Pin modes for ultrasonic
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Attach servos
  servo1.attach(2);
  servo2.attach(21);
  servo3.attach(22);
  servo4.attach(23);
  servo5.attach(15);
}

// --------------------------------------------------
// Main Loop
// --------------------------------------------------
void loop() {
  // Blink LED
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  delay(2000);

  // Check for commands from Serial
  if (Serial.available()) {
    char command = Serial.read();
    executeCommand(command);
  }

  // Check for commands from Bluetooth
  if (SerialBT.available()) {
    char command = SerialBT.read();
    executeCommand(command);
  }

  // Measure distance
  distanceCm = measureDistance();
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);

  // If object is too close, perform action
  if (distanceCm < 10) {
    action();
  }
}

// --------------------------------------------------
// Ultrasonic Measurement Function
// --------------------------------------------------
float measureDistance() {
  // Clear trigger
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);

  // Trigger pulse
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  // Measure echo time
  long duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate distance in cm
  float dist = duration * SOUND_SPEED / 2.0;

  // Limit to max distance if needed
  if (dist > MAX_DISTANCE) dist = MAX_DISTANCE;
  
  return dist;
}

// --------------------------------------------------
// Set Motors Helper
// --------------------------------------------------
void setMotors(bool r1, bool r2, bool l1, bool l2) {
  digitalWrite(right1, r1 ? HIGH : LOW);
  digitalWrite(right2, r2 ? HIGH : LOW);
  digitalWrite(left1,  l1 ? HIGH : LOW);
  digitalWrite(left2,  l2 ? HIGH : LOW);
}

// --------------------------------------------------
// Execute Command
// --------------------------------------------------
void executeCommand(char command) {
  switch (command) {
    case FORWARD:  // 'F'
      setMotors(true, false, true, false);
      Serial.println("Forward");
      break;

    case BACKWARD: // 'B'
      setMotors(false, true, false, true);
      Serial.println("Backward");
      break;

    case LEFT:     // 'L'
      setMotors(true, false, false, true);
      Serial.println("Left");
      break;

    case RIGHT:    // 'R'
      setMotors(false, true, true, false);
      Serial.println("Right");
      break;

    case PAUSE:    // 'P'
      setMotors(false, false, false, false);
      Serial.println("Pause");
      break;

    case CIRCLE:   // 'C'
      for (pos = 0; pos <= 90; pos++) { 
        servo1.write(pos);
        delay(15);
      }
      for (pos = 90; pos >= 0; pos--) {
        servo1.write(pos);
        delay(15);
        Serial.println("Circle Move: servo1");
      }
      break;

    case CROSS:    // 'X'
      for (pos = 45; pos <= 90; pos++) {
        servo2.write(pos);
        delay(15);
      }
      for (pos = 90; pos >= 45; pos--) {
        servo2.write(pos);
        delay(15);
        Serial.println("Cross Move: servo2");
      }
      for (pos = 45; pos >= 0; pos--) {
        servo2.write(pos);
        delay(15);
      }
      for (pos = 0; pos <= 45; pos++) {
        servo2.write(pos);
        delay(15);
      }
      Serial.println("Cross complete");
      break;

    case TRIANGLE: // 'T'
      for (pos = 0; pos <= 90; pos++) {
        servo3.write(pos);
        delay(15);
      }
      for (pos = 90; pos >= 0; pos--) {
        servo3.write(pos);
        delay(15);
        Serial.println("Triangle Move: servo3");
      }
      break;

    case SQUARE:   // 'S'
      servoAngle1 += 15; 
      if (servoAngle1 > 180) {
        servoAngle1 = 180;
      }
      servo1.write(servoAngle1);
      delay(20);
      Serial.println("Square Move: servo1");
      break;

    case START:    // 'A'
      servoAngle1 -= 15; 
      if (servoAngle1 < 0) {
        servoAngle1 = 0;
      }
      servo1.write(servoAngle1);
      delay(20);
      Serial.println("Start Move: servo1");
      break;

    default:
      // Invalid command => Stop
      setMotors(false, false, false, false);
      Serial.println("Invalid Command => Stop");
      break;
  }
}

// --------------------------------------------------
// Action Function
// --------------------------------------------------
void action() {
  // Move servo1 from 0 to 90
  for (pos = 0; pos <= 90; pos++) {
    servo1.write(pos);
    delay(15);
  }
  // Move servo2 from 0 to 90
  for (pos = 0; pos <= 90; pos++) {
    servo2.write(pos);
    delay(15);
  }
  // Move servo4 & servo5 from 0 to 90, then back to 0
  for (pos = 0; pos <= 90; pos++) {
    servo4.write(pos);
    servo5.write(pos);
    delay(15);
  }
  for (pos = 90; pos >= 0; pos--) {
    servo4.write(pos);
    servo5.write(pos);
    delay(15);
    Serial.println("Action: finger servo");
  }
  // Move servo3 (head) from 90 -> 180 -> 90 -> 0 -> 90
  for (posh = 90; posh <= 180; posh++) {
    servo3.write(posh);
    delay(15);
  }
  for (posh = 180; posh >= 90; posh--) {
    servo3.write(posh);
    delay(15);
    Serial.println("Action: head");
  }
  for (posh = 90; posh >= 70; posh--) {
    servo3.write(posh);
    delay(15);
    Serial.println("Action: head");
  }
  for (posh = 70; posh <= 90; posh++) {
    servo3.write(posh);
    delay(15);
  }
  // Return servo1 & servo2 from 90 to 0
  for (pos = 90; pos >= 0; pos--) {
    servo1.write(pos);
    servo2.write(pos);
    delay(15);
    Serial.println("Action: hand servo");
  }
}
