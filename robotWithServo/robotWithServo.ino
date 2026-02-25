/*
 * RC Car Control System with Three Servo Motors (Slow Servo Movement)
 * 
 * Features:
 * - Default state: Car stopped
 * - Command-based control using switch statements
 * - Three servo motors with slow, smooth movement patterns:
 *   - Servo 1: 0° → 90° → 0°
 *   - Servo 2: 0° → 90° → 0°
 *   - Servo 3: Initial position 45°, then 45° → 90° → 0° → 45°
 * - Motor control using only direction pins (no enable pins)
 *
 * Hardware requirements:
 * - Arduino Uno
 * - Motor driver or H-bridge for DC motors
 * - 3 servo motors
 * - Serial communication (Bluetooth module recommended for wireless control)
 */

#include <Servo.h>

// Create servo objects
Servo servo1;
Servo servo2;
Servo servo3;

// Servo pins
const int SERVO1_PIN = 11;
const int SERVO2_PIN = 10;
const int SERVO3_PIN = 9;

// Motor driver pins (no enable pins)
const int IN1 = 8;  // Input 1 for motor A
const int IN2 = 7;  // Input 2 for motor A
const int IN3 = 6;  // Input 1 for motor B
const int IN4 = 5;  // Input 2 for motor B

// Servo movement parameters
const int SERVO_DELAY = 20;     // Delay between each degree of movement (higher = slower)
const int PAUSE_DELAY = 500;    // Delay at each target position

// Variables to store received command
char command = 'S';  // Default to Stop

// Variables to store current servo positions
int servo1Pos = 90;
int servo2Pos = 0;
int servo3Pos = 45;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Attach servo motors to pins
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo3.attach(SERVO3_PIN);
  
  // Set initial positions
  servo1.write(servo1Pos);   // Servo 1 starts at 0 degrees
  servo2.write(servo2Pos);   // Servo 2 starts at 0 degrees
  servo3.write(servo3Pos);  // Servo 3 starts at 45 degrees
  
  // Set motor control pins as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Default state: Stop the car
  stopCar();
  
  Serial.println("RC Car Control System Initialized");
  Serial.println("Available Commands:");
  Serial.println("F: Forward, B: Backward, L: Left, R: Right");
  Serial.println("S: Stop (default)");
  Serial.println("1: Activate Servo 1");
  Serial.println("2: Activate Servo 2");
  Serial.println("3: Activate Servo 3");
}

void loop() {
  // Check if data is available to read
  if (Serial.available() > 0) {
    // Read the incoming byte
    command = Serial.read();
    
    // Process the command
    switch (command) {
      case 'F':  // Forward
        moveForward();
        Serial.println("Moving Forward");
        break;
        
      case 'B':  // Backward
        moveBackward();
        Serial.println("Moving Backward");
        break;
        
      case 'L':  // Left
        turnLeft();
        Serial.println("Turning Left");
        break;
        
      case 'R':  // Right
        turnRight();
        Serial.println("Turning Right");
        break;
        
      case '1':  // Activate Servo 1
        activateServo1();
        Serial.println("Activating Servo 1");
        break;
        
      case '2':  // Activate Servo 2
        activateServo2();
        Serial.println("Activating Servo 2");
        break;
        
      case '3':  // Activate Servo 3
        activateServo3();
        Serial.println("Activating Servo 3");
        break;
        
      case 'S':  // Stop (default)
      default:
        stopCar();
        Serial.println("Stopped");
        break;
    }
  }
  
  // Short delay for stability
  delay(50);
}

// Function to move servo slowly from current position to target position
void moveServoSlowly(Servo &servo, int &currentPos, int targetPos) {
  if (currentPos < targetPos) {
    // Move servo forward slowly
    for (int pos = currentPos; pos <= targetPos; pos++) {
      servo.write(pos);
      delay(SERVO_DELAY);
    }
  } else if (currentPos > targetPos) {
    // Move servo backward slowly
    for (int pos = currentPos; pos >= targetPos; pos--) {
      servo.write(pos);
      delay(SERVO_DELAY);
    }
  }
  
  // Update current position
  currentPos = targetPos;
  
  // Pause at target position
  delay(PAUSE_DELAY);
}

// Motor control functions (without enable pins)
void moveForward() {
  // Motor A - Forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  
  // Motor B - Forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward() {
  // Motor A - Backward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  
  // Motor B - Backward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  // Motor A - Backward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  
  // Motor B - Forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  // Motor A - Forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  
  // Motor B - Backward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopCar() {
  // Stop both motors by setting all control pins LOW
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// Servo control functions with slow movement
void activateServo1() {
  // Servo 1: 0° → 90° → 0°
  moveServoSlowly(servo1, servo1Pos, 0);
  moveServoSlowly(servo1, servo1Pos, 90);
}

void activateServo2() {
  // Servo 2: 0° → 90° → 0°
  moveServoSlowly(servo2, servo2Pos, 90);
  moveServoSlowly(servo2, servo2Pos, 0);
}

void activateServo3() {
  // Servo 3: 45° → 90° → 0° → 45°
  moveServoSlowly(servo3, servo3Pos, 90);
  moveServoSlowly(servo3, servo3Pos, 0);
  moveServoSlowly(servo3, servo3Pos, 45);
}