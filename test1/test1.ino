#include <SoftwareSerial.h>

SoftwareSerial BTSerial(6, 7); // RX (Receive) on pin 10, TX (Transmit) on pin 11
// Connect the TX pin of the Bluetooth module to the Arduino's pin 10 (RX) and the RX pin to pin 11 (TX).

// Motor Control Pins
const int motorPin1 = 2; // Motor 1 control pin 1
const int motorPin2 = 3; // Motor 1 control pin 2
const int motorPin3 = 4; // Motor 2 control pin 1
const int motorPin4 = 5; // Motor 2 control pin 2

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);

  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);

  stop(); // Stop the motors initially
}

void loop() {
  if (BTSerial.available()) {
    char command = BTSerial.read();
    executeCommand(command);
  }
}

void executeCommand(char command) {
  switch (command) {
    case 'F':
      forward();
      break;
    case 'B':
      backward();
      break;
    case 'L':
      left();
      break;
    case 'R':
      right();
      break;
    case 'S':
      stop();
      break;
  }
}

void forward() {
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);
  delay(1000);
}

void backward() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, HIGH);
}

void left() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
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
