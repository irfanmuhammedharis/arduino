#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Stepper.h>

// -------- SERVO CONFIG --------
Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40);

#define SERVOMIN           125
#define SERVOMAX           625
#define SERVO_COUNT        8
#define SERVO_HOME_ANGLE   0
#define SERVO_PICK_ANGLE   50

// -------- STEPPER CONFIG --------
const int stepsPerRevolution = 200;
const int totalRotations     = 10;
const int totalSteps         = stepsPerRevolution * totalRotations;
const int STEPPER_RPM        = 50;

Stepper myStepper(stepsPerRevolution, 8, 10, 9, 11);

// -------- SERIAL HANDLING --------
String serialBuffer = "";
bool busy = false;

// -------- SETUP --------
void setup() {
  Serial.begin(9600);
  Serial.println("System Booting...");

  board1.begin();
  board1.setPWMFreq(50);
  delay(10);

  // Initialize all servos to HOME
  Serial.println("Initializing servos to HOME (0°)");
  moveAllServos(SERVO_HOME_ANGLE);

  myStepper.setSpeed(STEPPER_RPM);

  Serial.println("System Ready. Waiting for PICK or PUT command...");
}

// -------- LOOP --------
void loop() {
  readSerial();
}

// -------- SERIAL READ --------
void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') continue;

    serialBuffer += c;
    serialBuffer.toUpperCase();

    if (serialBuffer.length() > 10) {
      serialBuffer.remove(0, serialBuffer.length() - 10);
    }

    if (!busy && serialBuffer.endsWith("PICK")) {
      serialBuffer = "";
      executePickSequence();
    }

    if (!busy && serialBuffer.endsWith("PUT")) {
      serialBuffer = "";
      executePutSequence();
    }
  }
}

// -------- PICK SEQUENCE --------
void executePickSequence() {
  busy = true;
  Serial.println("PICK sequence started");

  // 1️⃣ Stepper BACKWARD
  myStepper.step(-totalSteps);

  // 2️⃣ Servos → PICK
  moveAllServos(SERVO_PICK_ANGLE);
  delay(500);

  // 3️⃣ Stepper FORWARD
  myStepper.step(totalSteps);

  Serial.write("PLACE\n");
  Serial.println("PICK sequence completed");

  busy = false;
}

// -------- PUT SEQUENCE (REVERSE OF PICK) --------
void executePutSequence() {
  busy = true;
  Serial.println("PUT sequence started");

  // 1️⃣ Stepper BACKWARD
  myStepper.step(-totalSteps);

  // 2️⃣ Servos → HOME (RELEASE)
  moveAllServos(SERVO_HOME_ANGLE);
  delay(500);

  // 3️⃣ Stepper FORWARD
  myStepper.step(totalSteps);

  Serial.write("PUT_DONE\n");
  Serial.println("PUT sequence completed");

  busy = false;
}

// -------- HELPER FUNCTIONS --------
void moveAllServos(int angle) {
  for (int i = 0; i < SERVO_COUNT; i++) {
    board1.setPWM(i, 0, angleToPulse(angle));
  }
}

int angleToPulse(int ang) {
  ang = constrain(ang, 0, 180);
  return map(ang, 0, 180, SERVOMIN, SERVOMAX);
}
