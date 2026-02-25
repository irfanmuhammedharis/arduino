#include <ESP32Servo.h>

// Define pins for servos
Servo servo1;
Servo servo2;
const int servo1Pin = 18; // GPIO18
const int servo2Pin = 19; // GPIO19

// Define pins for gear motors
const int motor1Pin1 = 25; // GPIO25
const int motor1Pin2 = 26; // GPIO26
const int motor2Pin1 = 27; // GPIO27
const int motor2Pin2 = 32; // GPIO32

// Variables for non-blocking servo control
unsigned long previousMillis = 0;
const long interval = 20; // Interval between servo position updates (milliseconds)
int currentAngle1 = 0;
int currentAngle2 = 0;
int targetAngle1 = 0;
int targetAngle2 = 0;
bool movingServos = false;
unsigned long servoMoveStartTime = 0;
const unsigned long servoMoveDuration = 3000; // Duration to move from 0 to 90 degrees (milliseconds)

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("ESP32 is ready. Enter commands: A, L, R, F, B");

  // Attach servos to pins
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);

  // Set initial position of servos
  servo1.write(currentAngle1);
  servo2.write(currentAngle2);

  // Set motor pins as outputs
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);

  // Ensure motors are stopped initially
  stopMotors();
}

void loop() {
  // Check if data is available on the serial port
  if (Serial.available() > 0) {
    char command = Serial.read(); // Read the incoming command
    Serial.print("Received command: ");
    Serial.println(command);

    switch (command) {
      case 'A':
        // Set target angles for servos
        targetAngle1 = 90;
        targetAngle2 = 90;
        movingServos = true;
        servoMoveStartTime = millis();
        break;

      case 'L':
        // Turn left
        moveLeft();
        delay(2000);
        stopMotors();
        break;

      case 'R':
        // Turn right
        moveRight();
        delay(2000);
        stopMotors();
        break;

      case 'F':
        // Move forward
        moveForward();
        delay(2000);
        stopMotors();
        break;

      case 'B':
        // Move backward
        moveBackward();
        delay(2000);
        stopMotors();
        break;

      default:
        // Stop all motors if an unrecognized command is received
        stopMotors();
        Serial.println("Unrecognized command. Motors stopped.");
        break;
    }
  }

  // Update servo positions if movingServos is true
  if (movingServos) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      // Calculate progress
      unsigned long elapsedTime = currentMillis - servoMoveStartTime;
      if (elapsedTime <= servoMoveDuration) {
        // Map elapsed time to angle
        currentAngle1 = map(elapsedTime, 0, servoMoveDuration, 0, targetAngle1);
        currentAngle2 = map(elapsedTime, 0, servoMoveDuration, 0, targetAngle2);
        servo1.write(currentAngle1);
        servo2.write(currentAngle2);
      } else {
        // Movement complete
        movingServos = false;
        Serial.println("Servos reached target positions.");
        // Wait for 5 seconds before returning to 0 degrees
        delay(5000);
        // Set target angles to 0 degrees
        targetAngle1 = 0;
        targetAngle2 = 0;
        movingServos = true;
        servoMoveStartTime = millis();
      }
    }
  }
}

void moveForward() {
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
  Serial.println("Moving forward.");
}

void moveBackward() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  Serial.println("Moving backward.");
}

void moveLeft() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
  Serial.println("Turning left.");
}

void moveRight() {
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  Serial.println("Turning right.");
}

void stopMotors() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);
  Serial.println("Motors stopped.");
}
