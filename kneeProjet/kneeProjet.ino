#include <Servo.h>

/* ===================== Hardware ===================== */
#define ENC_CLK     2     // INT0
#define ENC_DT      3
#define SERVO_PIN   8

/* ===================== Encoder ===================== */
volatile long encoderCount = 0;
volatile bool encoderMoved = false;

/* ===================== Servo ===================== */
Servo servo;
const int SERVO_CENTER = 90;
const int SERVO_MIN = 0;
const int SERVO_MAX = 180;

const float DEG_PER_STEP = 3.0f;

/* Timing */
const unsigned long SERVO_STEP_INTERVAL = 15;
const unsigned long HOLD_TIME = 800;

/* ===================== State Machine ===================== */
enum State {
  IDLE,
  RECORDING,
  SERVO_MOVE_OUT,
  SERVO_HOLD,
  SERVO_RETURN
};

State state = IDLE;
State lastState = IDLE;

/* ===================== Runtime ===================== */
long recordedSteps = 0;
int servoTarget = SERVO_CENTER;
int servoPosition = SERVO_CENTER;

unsigned long lastServoUpdate = 0;
unsigned long holdStartTime = 0;

/* ===================== Encoder ISR ===================== */
void encoderISR() {
  bool dt = digitalRead(ENC_DT);
  encoderCount += dt ? -1 : 1;
  encoderMoved = true;
}

/* ===================== Debug Helpers ===================== */
void printState(State s) {
  switch (s) {
    case IDLE:           Serial.println(F("[STATE] IDLE")); break;
    case RECORDING:      Serial.println(F("[STATE] RECORDING")); break;
    case SERVO_MOVE_OUT: Serial.println(F("[STATE] SERVO_MOVE_OUT")); break;
    case SERVO_HOLD:     Serial.println(F("[STATE] SERVO_HOLD")); break;
    case SERVO_RETURN:   Serial.println(F("[STATE] SERVO_RETURN")); break;
  }
}

/* ===================== Setup ===================== */
void setup() {
  Serial.begin(115200);
  while (!Serial);   // Safe on Uno (ignored)

  Serial.println(F("\n=== SYSTEM BOOT ==="));

  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);

  servo.attach(SERVO_PIN);
  servo.write(SERVO_CENTER);

  attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderISR, CHANGE);

  Serial.println(F("Encoder: INT0 (pin 2)"));
  Serial.println(F("Servo  : pin 8"));
  Serial.println(F("==================="));
}

/* ===================== Main Loop ===================== */
void loop() {

  /* State change debug */
  if (state != lastState) {
    printState(state);
    lastState = state;
  }

  switch (state) {

    case IDLE:
      if (encoderMoved && encoderCount != 0) {
        Serial.print(F("[ENC] Movement detected, count="));
        Serial.println(encoderCount);
        encoderMoved = false;
        state = RECORDING;
      }
      break;

    case RECORDING:
      if (abs(encoderCount) > 1) {
        recordedSteps = encoderCount;
      }

      if (encoderCount == 0 && recordedSteps != 0) {
        Serial.print(F("[REC] Recorded steps = "));
        Serial.println(recordedSteps);

        servoTarget = SERVO_CENTER +
                      (int)(recordedSteps * DEG_PER_STEP);
        servoTarget = constrain(servoTarget, SERVO_MIN, SERVO_MAX);

        Serial.print(F("[SERVO] Target angle = "));
        Serial.println(servoTarget);

        state = SERVO_MOVE_OUT;
      }
      break;

    case SERVO_MOVE_OUT:
      if (updateServoTowards(servoTarget)) {
        Serial.println(F("[SERVO] Target reached"));
        holdStartTime = millis();
        state = SERVO_HOLD;
      }
      break;

    case SERVO_HOLD:
      if (millis() - holdStartTime >= HOLD_TIME) {
        Serial.println(F("[SERVO] Hold complete"));
        state = SERVO_RETURN;
      }
      break;

    case SERVO_RETURN:
      if (updateServoTowards(SERVO_CENTER)) {
        Serial.println(F("[SERVO] Returned to center"));
        resetSystem();
      }
      break;
  }
}

/* ===================== Servo Motion ===================== */
bool updateServoTowards(int target) {
  unsigned long now = millis();
  if (now - lastServoUpdate < SERVO_STEP_INTERVAL) return false;
  lastServoUpdate = now;

  if (servoPosition < target) servoPosition++;
  else if (servoPosition > target) servoPosition--;
  else return true;

  servo.write(servoPosition);
  return false;
}

/* ===================== Reset ===================== */
void resetSystem() {
  noInterrupts();
  encoderCount = 0;
  encoderMoved = false;
  interrupts();

  recordedSteps = 0;
  state = IDLE;

  Serial.println(F("[SYSTEM] Reset complete"));
}
