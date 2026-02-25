#include <BluetoothSerial.h>
#include <Servo.h>  // Official Servo library

BluetoothSerial SerialBT;  // Bluetooth object
  
// Motor Control Pins
const int motorPin1 = 33;
const int motorPin2 = 25;
const int motorPin3 = 26;
consc:\Users\91964\Documents\Arduino\final_sketch_metalt int motorPin4 = 27;

// Servo Pins
const int servo1Pin = 12;  // Gripper
const int servo2Pin = 13;  // Arm lift

// Servo Objects
Servo servo1;
Servo servo2;

// Pick Sequence Variables
unsigned long pickStartTime = 0;
bool pickInProgress = false;
const unsigned long pickPauseMs = 1000;  // 1s pause

void setup() {

  Serial.begin(115200);
  SerialBT.begin("BluetoothCar");

  // Motor pins
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);

  // Attach servos
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);

  // Initial position
  servo1.write(0);
  servo2.write(0);

  stop();

  Serial.println("Setup complete. Ready for Bluetooth commands.");
}

void loop() {

  // Bluetooth input
  if (SerialBT.available()) {
    char command = SerialBT.read();

    Serial.print("Received command: ");
    Serial.println(command);

    executeCommand(command);
  }

  // Non-blocking pick sequence
  if (pickInProgress) {

    unsigned long currentTime = millis();

    if (currentTime - pickStartTime >= pickPauseMs) {

      servo1.write(0);
      servo2.write(0);

      pickInProgress = false;

      Serial.println("Pick sequence complete.");
    }
  }
}

void executeCommand(char command) {

  switch (command) {

    case 'F':
      forward();
      Serial.println("Moving forward.");
      break;

    case 'B':
      backward();
      Serial.println("Moving backward.");
      break;

    case 'L':
      left();
      Serial.println("Turning left.");
      break;

    case 'R':
      right();
      Serial.println("Turning right.");
      break;

    case 'S':
      stop();
      Serial.println("Stopped.");
      break;

    case 'P':
      pick();
      Serial.println("Pick sequence started.");
      break;

    default:
      Serial.println("Invalid command.");
      break;
  }
}

// Motor Functions
void forward() {
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);
}

void backward() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, HIGH);
}

void left() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);
}

void right() {
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
}

void stop() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
}

// Pick Function
void pick() {

  if (!pickInProgress) {

    servo1.write(120);  // Gripper opens
    servo2.write(15);   // Arm lowers

    pickStartTime = millis();
    pickInProgress = true;

    Serial.println("Servos moved to pick position.");
  }
}
