const int leftMotorDirPin = 7;
const int leftMotorPwmPin = 6;
const int rightMotorDirPin = 4;
const int rightMotorPwmPin = 5;

// Pin definitions for auxiliary motors
const int auxMotor1Pin = 8;
const int auxMotor2Pin = 9;

// Speed settings
 int normalSpeed = 180;  // Normal driving speed (0-255)
const int turnSpeed = 150;    // Turning speed (0-255)

char command;                 // Variable to store received command

void setup() {
  // Initialize motor control pins
  pinMode(leftMotorDirPin, OUTPUT);
  pinMode(leftMotorPwmPin, OUTPUT);
  pinMode(rightMotorDirPin, OUTPUT);
  pinMode(rightMotorPwmPin, OUTPUT);
  pinMode(auxMotor1Pin, OUTPUT);
  pinMode(auxMotor2Pin, OUTPUT);
  
  // Initialize all motors to stopped state
  digitalWrite(auxMotor1Pin, LOW);
  digitalWrite(auxMotor2Pin, LOW);
  analogWrite(leftMotorPwmPin, 0);
  analogWrite(rightMotorPwmPin, 0);
  
  // Initialize Bluetooth serial communication
  Serial.begin(9600);
  Serial.println("Bluetooth RC Car Ready");
}

void loop() {
  // Check if data is available from Bluetooth
  if (Serial.available() > 0) {
    command = Serial.read();
    executeCommand(command);
  }
}

void executeCommand(char cmd) {
  switch(cmd) {
    case 'F':  // Move forward
      moveForward();
      break;
    case 'B':  // Move backward
      moveBackward();
      break;
    case 'L':  // Turn left
      turnLeft();
      break;
    case 'R':  // Turn right
      turnRight();
      break;
    case 'S':  // Stop all drive motors
      stopMotors();
      break;
    case 'X':  // Start auxiliary motor 1
      digitalWrite(auxMotor1Pin, HIGH);
      Serial.println("Auxiliary Motor 1 Started");
      break;
    case 'x':  // Stop auxiliary motor 1
      digitalWrite(auxMotor1Pin, LOW);
      Serial.println("Auxiliary Motor 1 Stopped");
      break;
    case 'Y':  // Start auxiliary motor 2
      digitalWrite(auxMotor2Pin, HIGH);
      Serial.println("Auxiliary Motor 2 Started");
      break;
    case 'y':  // Stop auxiliary motor 2
      digitalWrite(auxMotor2Pin, LOW);
      Serial.println("Auxiliary Motor 2 Stopped");
      break;
      case '100':
      normalSpeed = 255;
      break;
      case '50':
      normalSpeed = 125;
      break;
      case '75':
      normalSpeed = 180;
      break;
    default:
      // Unrecognized command - do nothing
      break;
  }
}

void moveForward() {
  // Set direction pins
  digitalWrite(leftMotorDirPin, HIGH);
  digitalWrite(rightMotorDirPin, HIGH);
  
  // Set speed
  analogWrite(leftMotorPwmPin, normalSpeed);
  analogWrite(rightMotorPwmPin, normalSpeed);
  
  Serial.println("Moving Forward");
}

void moveBackward() {
  // Set direction pins
  digitalWrite(leftMotorDirPin, LOW);
  digitalWrite(rightMotorDirPin, LOW);
  
  // Set speed
  analogWrite(leftMotorPwmPin, normalSpeed);
  analogWrite(rightMotorPwmPin, normalSpeed);
  
  Serial.println("Moving Backward");
}

void turnLeft() {
  // Left motor backward, right motor forward
  digitalWrite(leftMotorDirPin, LOW);
  digitalWrite(rightMotorDirPin, HIGH);
  
  // Set speeds
  analogWrite(leftMotorPwmPin, turnSpeed);
  analogWrite(rightMotorPwmPin, turnSpeed);
  
  Serial.println("Turning Left");
}

void turnRight() {
  // Left motor forward, right motor backward
  digitalWrite(leftMotorDirPin, HIGH);
  digitalWrite(rightMotorDirPin, LOW);
  
  // Set speeds
  analogWrite(leftMotorPwmPin, turnSpeed);
  analogWrite(rightMotorPwmPin, turnSpeed);
  
  Serial.println("Turning Right");
}

void stopMotors() {
  // Stop both motors
  analogWrite(leftMotorPwmPin, 0);
  analogWrite(rightMotorPwmPin, 0);
  
  Serial.println("Motors Stopped");
}