#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ESP32Servo.h>  // Include the ESP32Servo library instead of Servo
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

Servo myservoh;
Servo myservor;
Servo myservol;

#define right1 12
#define right2 14
#define left1 27
#define left2 26  // Use GPIO 19 for ESP32 instead of A5
#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define MAX_DEVICES 2 
#define CLK_PIN 18 //yellow
#define DATA_PIN 23 //white
unsigned long previousMillis = 0;
const long interval = 500;
const int irSensorLeft = 32;    // Left IR sensor
const int irSensorRight = 33;   // Right IR sensor
bool eye1Active = true;
bool lineFollowingMode = false;
#define CS_PIN_1 15 //blue
#define CS_PIN_2 15
MD_MAX72XX mx[] = {
  MD_MAX72XX(HARDWARE_TYPE, CS_PIN_1, 1),
  MD_MAX72XX(HARDWARE_TYPE, CS_PIN_2, 1),
};

int posh = 90;
int posr = 0;
int posl = 0;
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
const uint8_t spooky_eye[8] = { 0x3C,
                                0x7E,
                                0xFF,
                                0xFF,
                                0xFF,
                                0xFF,
                                0x7E,
                                0x3C };
const uint8_t closed_eye_up[8] = { 0x00,
                                   0x0C,
                                   0x18,
                                   0x18,
                                   0x18,
                                   0x18,
                                   0x0C,
                                   0x00 };
const uint8_t closed_eye_down[8] = { 0x00,
                                     0x0C,
                                     0x0C,
                                     0x06,
                                     0x06,
                                     0x0C,
                                     0x0C,
                                     0x00 };


void setup() {
  Serial.begin(9600);
  SerialBT.begin("CAMPUS MITHRA V1.0.3");
  delay(500);

  // Attach servos to corresponding GPIO pins
  myservoh.attach(22);
  myservor.attach(21);
  myservol.attach(2);
pinMode(irSensorLeft,INPUT);
pinMode(irSensorRight,INPUT);
  pinMode(right1, OUTPUT);
  pinMode(right2, OUTPUT);
  pinMode(left1, OUTPUT);  pinMode(left2, OUTPUT);
  for (int i = 0; i < MAX_DEVICES; i++) {
    mx[i].begin();
    mx[i].control(MD_MAX72XX::INTENSITY, MAX_INTENSITY / 2);
    mx[i].control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    mx[i].clear();
  }
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
}

void loop() {

    unsigned long currentMillis = millis();  // Get the current time

  // Check if it's time to switch eye animations
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Toggle between eye animations
    if (eye1Active) {
      eye1();
    } else {
      eye2();
    }
    eye1Active = !eye1Active;  // Toggle the flag
  }
  if (SerialBT.available()) {
    char val = SerialBT.read();
    switch (val) {
      case 'A':  // Activate Line Following Mode
        lineFollowingMode = true;
        break;
      case 'D':  // Deactivate Line Following Mode
        lineFollowingMode = false;
        stopRobot();
        break;
      
      case 'F':
        digitalWrite(right1, HIGH);
        digitalWrite(right2, LOW);
        digitalWrite(left1, HIGH);
        digitalWrite(left2, LOW);
        delay(3000);
        digitalWrite(right1, LOW);
        digitalWrite(right2, LOW);
        digitalWrite(left1, LOW);
        digitalWrite(left2, LOW);
        break;
        case 'Z':
        digitalWrite(right1, HIGH);
        digitalWrite(right2, LOW);
        digitalWrite(left1, HIGH);
        digitalWrite(left2, LOW);
        delay(3000);
        stopRobot();
        delay(3000);
        digitalWrite(right1, LOW);
        digitalWrite(right2, HIGH);
        digitalWrite(left1, LOW);
        digitalWrite(left2, HIGH);
        delay(3000);
        stopRobot();
        break;
      case 'B':
        digitalWrite(right1, LOW);
        digitalWrite(right2, HIGH);
        digitalWrite(left1, LOW);
        digitalWrite(left2, HIGH);
        delay(3000);
        digitalWrite(right1, LOW);
        digitalWrite(right2, LOW);
        digitalWrite(left1, LOW);
        digitalWrite(left2, LOW);
        break;
      case 'R':
        digitalWrite(right1, HIGH);
        digitalWrite(right2, LOW);
        digitalWrite(left1, LOW);
        digitalWrite(left2, HIGH);
        delay(300);
        digitalWrite(right1, LOW);
        digitalWrite(right2, LOW);
        digitalWrite(left1, LOW);
        digitalWrite(left2, LOW);
        break;
      case 'L':
        digitalWrite(right1, LOW);
        digitalWrite(right2, HIGH);
        digitalWrite(left1, HIGH);
        digitalWrite(left2, LOW);
        delay(300);
        digitalWrite(right1, LOW);
        digitalWrite(right2, LOW);
        digitalWrite(left1, LOW);
        digitalWrite(left2, LOW);
        break;
      case 'S':
        Serial.println("stop");
        digitalWrite(right1, LOW);
        digitalWrite(right2, LOW);
        digitalWrite(left1, LOW);
        digitalWrite(left2, LOW);
        break;
      case '5':
        Serial.print("head");
        for (posh = 90; posh <= 180; posh += 1) {
          myservoh.write(posh);
          delay(15);
        }
        for (posh = 180; posh >= 0; posh -= 1) {
          myservoh.write(posh);
          delay(15);
        }
        for (posh = 0; posh <= 90; posh += 1) {
          myservoh.write(posh);
          delay(15);
        }
        break;
      case '6':
        Serial.println("right hand");
        for (posr = 0; posr <= 180; posr += 1) {
          myservor.write(posr);
          delay(15);
        }
        for (posr = 180; posr >= 0; posr -= 1) {
          myservor.write(posr);
          delay(15);
        }
        break;
      case '7':
        Serial.println("left hand");
        for (posl = 0; posl <= 180; posl += 1) {
          myservol.write(posl);
          delay(15);
        }
        for (posl = 180; posl >= 0; posl -= 1) {
          myservol.write(posl);
          delay(15);
        }
        break;
      case '1':
        Serial.println("mix");
        for (posh = 90, posr = 0, posl = 0; posh <= 180 && posr <= 180 && posl <= 180; posh += 1, posr += 1, posl += 1) {
          myservoh.write(posh);
          myservor.write(posr);
          myservol.write(posl);
          delay(15);
        }
        for (posh = 180; posh >= 0; posh -= 1) {
          myservoh.write(posh);
          delay(15);
        }
        for (posh = 0, posr = 180, posl = 180; posh <= 90 && posr >= 0 && posl >= 0; posh += 1, posr -= 1, posl -= 1) {
          myservoh.write(posh);
          myservor.write(posr);
          myservol.write(posl);
          delay(15);
        }
        break;
      case '2':
        Serial.println("mix2");
        for (posh = 90, posr = 0; posh >= 0 && posr <= 180; posh -= 1, posr += 1) {
          myservoh.write(posh);
          myservor.write(posr);
          myservol.write(0);
          delay(15);
        }
        for (posh = 180, posr = 180, posl = 0; posh >= 90 && posr >= 90 && posl <= 90; posh -= 1, posr -= 1, posl += 1) {
          myservoh.write(posh);
          myservor.write(posr);
          myservol.write(posl);
          delay(15);
        }
        for (posh = 90, posr = 90, posl = 90; posh >= 0 && posr >= 0 && posl <= 180; posh -= 1, posr -= 1, posl += 1){
          myservoh.write(posh);
        myservor.write(posr);
        myservol.write(posl);
        delay(15);
    }
    break;
  }
  if(lineFollowingMode){
    followLine();
  }
}
}

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
void followLine() {
  Serial.println("Line follower started");
  int leftSensorValue = digitalRead(irSensorLeft);
  int rightSensorValue = digitalRead(irSensorRight);

  if (leftSensorValue == LOW && rightSensorValue == LOW) {
    // Both sensors detect the line - move forward
    moveForward();
  } else if (leftSensorValue == LOW && rightSensorValue == HIGH) {
    // Only the left sensor detects the line - turn right
    turnRight();
    Serial.println("right");
  } else if (leftSensorValue == HIGH && rightSensorValue == LOW) {
    // Only the right sensor detects the line - turn left
    turnLeft();
    Serial.println("left");
  } else {
    // Neither sensor detects the line - stop or move forward depending on your line-following strategy
    stopRobot();
  }
}
void moveForward(){
  digitalWrite(right1, HIGH);
        digitalWrite(right2, LOW);
        digitalWrite(left1, HIGH);
        digitalWrite(left2, LOW);
}

void turnRight(){
 digitalWrite(right1, HIGH);
        digitalWrite(right2, LOW);
        digitalWrite(left1, LOW);
        digitalWrite(left2, HIGH);
}
void turnLeft(){
   digitalWrite(right1, LOW);
        digitalWrite(right2, HIGH);
        digitalWrite(left1, HIGH);
        digitalWrite(left2, LOW);
}
void stopRobot(){
  Serial.println("stop");
 digitalWrite(right1, LOW);
        digitalWrite(right2, LOW);
        digitalWrite(left1, LOW);
        digitalWrite(left2, LOW);
}

