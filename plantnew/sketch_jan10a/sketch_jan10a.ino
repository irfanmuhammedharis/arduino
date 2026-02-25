#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Stepper.h>

/* ================= SERVO CONFIG ================= */
Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40);

#define SERVOMIN     125
#define SERVOMAX     625
#define SERVO_COUNT  8

/* ================= STEPPER CONFIG ================= */
// Typical 1.8° stepper motor
const int stepsPerRevolution = 200;
const int totalRotations = 10;
const int totalSteps = stepsPerRevolution * totalRotations;

// Arduino Mega digital pins (valid only on Mega)
#define STEPPER_PIN_1 8
#define STEPPER_PIN_2 9
#define STEPPER_PIN_3 10
#define STEPPER_PIN_4 11

Stepper myStepper(
  stepsPerRevolution,
  STEPPER_PIN_1,
  STEPPER_PIN_3,
  STEPPER_PIN_2,
  STEPPER_PIN_4
);

/* ================================================= */

void setup() {
  Serial.begin(9600);
  Serial.println(F("Arduino Mega Servo + Stepper Controller Ready"));
  Serial.println(F("1: All servos -> 50°"));
  Serial.println(F("2: Servos one by one -> 0°"));
  Serial.println(F("3: Stepper CW 10 rotations"));
  Serial.println(F("4: Stepper CCW 10 rotations"));

  // PCA9685 Servo Driver Init
  board1.begin();
  board1.setPWMFreq(60);   // Analog servos: ~50–60 Hz
  delay(10);

  // Stepper speed (RPM)
  myStepper.setSpeed(50);
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {

      case '1':
        Serial.println(F("All servos -> 50 degrees"));
        for (int i = 0; i < SERVO_COUNT; i++) {
          board1.setPWM(i, 0, angleToPulse(50));
        }
        break;

      case '2':
        Serial.println(F("Servos moving one by one -> 0 degrees"));
        for (int i = 0; i < SERVO_COUNT; i++) {
          board1.setPWM(i, 0, angleToPulse(0));
          delay(3000);
        }
        break;

      case '3':
        Serial.println(F("Stepper rotating CW 10 rotations"));
        myStepper.step(totalSteps);
        break;

      case '4':
        Serial.println(F("Stepper rotating CCW 10 rotations"));
        myStepper.step(-totalSteps);
        break;
    }
  }
}

/* ================= ANGLE TO PWM ================= */
int angleToPulse(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}
