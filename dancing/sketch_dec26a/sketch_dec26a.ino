#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ESP32Servo.h>  // For servo control

// -------------------------------
// CONFIGURATION
// -------------------------------
#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define MAX_DEVICES 2
#define CLK_PIN 18  // LED Matrix clock pin
#define DATA_PIN 23 // LED Matrix data pin
#define CS_PIN_1 15
#define CS_PIN_2 15

// Motor driver pins
#define right1 12
#define right2 14
#define left1 27
#define left2 26

// Servos
Servo myservoh;  // head servo
Servo myservor;  // right arm servo
Servo myservol;  // left arm servo

// Eye animation arrays
const uint8_t eye[8] = {
  0x00,
  0x18,
  0x3C,
  0x7E,
  0x7E,
  0x3C,
  0x18,
  0x00
};

const uint8_t spooky_eye[8] = {
  0x3C,
  0x7E,
  0xFF,
  0xFF,
  0xFF,
  0xFF,
  0x7E,
  0x3C
};

// Create an array of MD_MAX72XX objects
MD_MAX72XX mx[] = {
  MD_MAX72XX(HARDWARE_TYPE, CS_PIN_1, 1),
  MD_MAX72XX(HARDWARE_TYPE, CS_PIN_2, 1),
};

bool eye1Active = true;
unsigned long previousMillis = 0;
const long interval = 500;  // Blinking interval (ms)

// -------------------------------
// SETUP
// -------------------------------
void setup() {
  Serial.begin(9600);
  
  // Initialize and attach servos
  myservoh.attach(22);
  myservor.attach(21);
  myservol.attach(2);

  // Motor pins as outputs
  pinMode(right1, OUTPUT);
  pinMode(right2, OUTPUT);
  pinMode(left1, OUTPUT);
  pinMode(left2, OUTPUT);

  // Initialize LED matrices
  for (int i = 0; i < MAX_DEVICES; i++) {
    mx[i].begin();
    mx[i].control(MD_MAX72XX::INTENSITY, MAX_INTENSITY / 2);
    mx[i].control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    mx[i].clear();
  }

  // Brief "blink" on startup
  eye1();
  delay(500);
  eye2();
  delay(500);
  eye1();
  delay(500);
  eye2();
  delay(500);
  eye1();
  delay(500);
  eye2();
  delay(500);

  Serial.println("Dancing Robot Ready!");
}

// -------------------------------
// MAIN LOOP
// -------------------------------
void loop() {
  // Handle eye-blinking animation
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (eye1Active) {
      eye1();
    } else {
      eye2();
    }
    eye1Active = !eye1Active;  // Toggle eye state
  }

  // Run the dance routine continuously
  danceRoutine();
}

// -------------------------------
// EYE ANIMATION FUNCTIONS
// -------------------------------
void eye1() {
  mx[0].clear();
  for (uint8_t row = 0; row < 8; row++) {
    mx[0].setRow(row, eye[row]);
  }
  mx[1].clear();
  for (uint8_t row = 0; row < 8; row++) {
    mx[1].setRow(row, eye[row]);
  }
}

void eye2() {
  mx[0].clear();
  for (uint8_t row = 0; row < 8; row++) {
    mx[0].setRow(row, spooky_eye[row]);
  }
  mx[1].clear();
  for (uint8_t row = 0; row < 8; row++) {
    mx[1].setRow(row, spooky_eye[row]);
  }
}

// -------------------------------
// DANCE ROUTINE
// -------------------------------
void danceRoutine() {
  // Example: Head nod + arms wave + short forward/back motions

  // 1. Nod head up and down
  for (int pos = 90; pos <= 140; pos++) {
    myservoh.write(pos);
    delay(15);
  }
  for (int pos = 140; pos >= 40; pos--) {
    myservoh.write(pos);
    delay(15);
  }
  for (int pos = 40; pos <= 90; pos++) {
    myservoh.write(pos);
    delay(15);
  }

  // 2. Wave right arm
  for (int posr = 0; posr <= 60; posr++) {
    myservor.write(posr);
    delay(15);
  }
  for (int posr = 60; posr >= 0; posr--) {
    myservor.write(posr);
    delay(15);
  }

  // 3. Wave left arm
  for (int posl = 0; posl <= 60; posl++) {
    myservol.write(posl);
    delay(15);
  }
  for (int posl = 60; posl >= 0; posl--) {
    myservol.write(posl);
    delay(15);
  }

  // 4. Small forward movement
  moveForward();
  delay(800);
  stopRobot();
  delay(300);

  // 5. Small backward movement
  moveBackward();
  delay(800);
  stopRobot();
  delay(300);

  // Repeat or add more steps as desired
}

// -------------------------------
// HELPER FUNCTIONS
// -------------------------------
void moveForward() {
  digitalWrite(right1, HIGH);
  digitalWrite(right2, LOW);
  digitalWrite(left1, HIGH);
  digitalWrite(left2, LOW);
}

void moveBackward() {
  digitalWrite(right1, LOW);
  digitalWrite(right2, HIGH);
  digitalWrite(left1, LOW);
  digitalWrite(left2, HIGH);
}

void stopRobot() {
  digitalWrite(right1, LOW);
  digitalWrite(right2, LOW);
  digitalWrite(left1, LOW);
  digitalWrite(left2, LOW);
}