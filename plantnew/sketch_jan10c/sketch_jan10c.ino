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
const int STEPPER_RPM        = 50;

Stepper myStepper(stepsPerRevolution, 8, 11, 9, 10);

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

  // Initialize all servos to home
  for (int i = 0; i < SERVO_COUNT; i++) {
    board1.setPWM(i, 0, angleToPulse(SERVO_HOME_ANGLE));
  }

  myStepper.setSpeed(STEPPER_RPM);

  Serial.println("READY");
}

// -------- LOOP --------
void loop() {
  readHardwareSerial();
}

// -------- READ SERIAL --------
void readHardwareSerial() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      processCommand(serialBuffer);
      serialBuffer = "";
    } else {
      serialBuffer += c;
    }
  }
}

// -------- PROCESS COMMAND --------
void processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (busy) return;

  // PICK command
  if (cmd == "PICK") {
    executePickSequence();
    return;
  }

  // Numeric stepper command
  int rotations = cmd.toInt();

  if (rotations != 0) {
    rotateStepper(rotations);
  }
}

// -------- ROTATE STEPPER --------
void rotateStepper(int rotations) {
  busy = true;

  long steps = (long)rotations * stepsPerRevolution;
  myStepper.step(steps);

  Serial.print("DONE: ");
  Serial.println(rotations);

  busy = false;
}

// -------- PICK SEQUENCE --------
void executePickSequence() {
  busy = true;

  myStepper.step(-stepsPerRevolution * 10);

  for (int i = 0; i < SERVO_COUNT; i++) {
    board1.setPWM(i, 0, angleToPulse(SERVO_PICK_ANGLE));
  }

  delay(500);

  myStepper.step(stepsPerRevolution * 10);

  Serial.println("PLACE");

  busy = false;
}

// -------- ANGLE TO PWM --------
int angleToPulse(int ang) {
  ang = constrain(ang, 0, 180);
  return map(ang, 0, 180, SERVOMIN, SERVOMAX);
}
