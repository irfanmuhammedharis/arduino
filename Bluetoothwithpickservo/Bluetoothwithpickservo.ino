#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

/* ================= MOTOR PINS ================= */
#define M1A 33
#define M1B 25
#define M2A 26
#define M2B 27

/* ================= SERVO PINS ================= */
#define SERVO1_PIN 12   // Gripper
#define SERVO2_PIN 13   // Arm

/* ================= LEDC SETTINGS ================= */
#define SERVO_FREQ 50        // 50 Hz for servo
#define SERVO_RES 16         // 16-bit resolution
#define SERVO1_CH 0
#define SERVO2_CH 1

/* ================= PICK SEQUENCE ================= */
bool pickInProgress = false;
unsigned long pickStartTime = 0;
const unsigned long PICK_DELAY = 1000;

/* ================= UTILS ================= */
uint32_t angleToDuty(int angle) {
  // 0.5ms–2.5ms pulse width at 50Hz, 16-bit
  return map(angle, 0, 180, 1638, 8192);
}

void servoWrite(uint8_t channel, int angle) {
  ledcWrite(channel, angleToDuty(angle));
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  SerialBT.begin("BluetoothCar");

  pinMode(M1A, OUTPUT);
  pinMode(M1B, OUTPUT);
  pinMode(M2A, OUTPUT);
  pinMode(M2B, OUTPUT);

  // Attach servos using new ESP32 LEDC API
  ledcAttach(SERVO1_PIN, SERVO_FREQ, SERVO_RES);
  ledcAttach(SERVO2_PIN, SERVO_FREQ, SERVO_RES);

  servoWrite(SERVO1_CH, 0);
  servoWrite(SERVO2_CH, 0);

  stopMotors();

  Serial.println("ESP32 ready (core 3.x compatible)");
}

/* ================= LOOP ================= */
void loop() {
  if (SerialBT.available()) {
    char cmd = SerialBT.read();
    handleCommand(cmd);
  }

  if (pickInProgress && millis() - pickStartTime >= PICK_DELAY) {
    servoWrite(SERVO1_CH, 0);
    servoWrite(SERVO2_CH, 0);
    pickInProgress = false;
    Serial.println("Pick complete");
  }
}

/* ================= COMMAND HANDLER ================= */
void handleCommand(char cmd) {
  switch (cmd) {
    case 'F': forward(); break;
    case 'B': backward(); break;
    case 'L': left(); break;
    case 'R': right(); break;
    case 'S': stopMotors(); break;
    case 'P': startPick(); break;
    default: Serial.println("Invalid command");
  }
}

/* ================= MOTOR CONTROL ================= */
void forward() {
  digitalWrite(M1A, HIGH);
  digitalWrite(M1B, LOW);
  digitalWrite(M2A, HIGH);
  digitalWrite(M2B, LOW);
}

void backward() {
  digitalWrite(M1A, LOW);
  digitalWrite(M1B, HIGH);
  digitalWrite(M2A, LOW);
  digitalWrite(M2B, HIGH);
}

void left() {
  digitalWrite(M1A, LOW);
  digitalWrite(M1B, LOW);
  digitalWrite(M2A, HIGH);
  digitalWrite(M2B, LOW);
}

void right() {
  digitalWrite(M1A, HIGH);
  digitalWrite(M1B, LOW);
  digitalWrite(M2A, LOW);
  digitalWrite(M2B, LOW);
}

void stopMotors() {
  digitalWrite(M1A, LOW);
  digitalWrite(M1B, LOW);
  digitalWrite(M2A, LOW);
  digitalWrite(M2B, LOW);
}

/* ================= PICK SEQUENCE ================= */
void startPick() {
  if (!pickInProgress) {
    servoWrite(SERVO1_CH, 120);  // Open gripper
    servoWrite(SERVO2_CH, 15);   // Lower arm
    pickStartTime = millis();
    pickInProgress = true;
    Serial.println("Pick started");
  }
}
                              