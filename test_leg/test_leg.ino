#include <Encoder.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define pins for rotary encoder switch and encoder pins
const int SW_PIN = 4;
const int PIN_A = 2;
const int PIN_B = 3;

Encoder myEnc(PIN_A, PIN_B);

// Define servo variables
Servo myservo;
const int servoPin = 9; // Servo signal pin
const int homePosition = 90; // Initial position
const int stepValue = 3; // Rotation step value

// Define LCD variables
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Global variables
int servoAngle = homePosition;
int maxAngle = 0;
bool returnToHome = false;

void setup() {
  Serial.begin(9600);
  
  pinMode(SW_PIN, INPUT);
  
  myservo.attach(servoPin);
  myservo.write(servoAngle); // Move servo to initial position

  lcd.init();
  lcd.backlight();
  lcd.print("Encoder Servo");
  lcd.setCursor(0, 1);
  lcd.print("Angle: ");
  delay(2000);
  lcd.clear();
  lcd.print("Encoder Servo");
  lcd.setCursor(0, 1);
  lcd.print("Angle: ");
}

void loop() {
  long newPosition = myEnc.read();
  
  if (!returnToHome) {
    if (abs(newPosition - servoAngle) > 3) {
      if (newPosition > servoAngle) {
        servoAngle += stepValue;
        if (servoAngle > 180)
          servoAngle = 180;
      } else {
        servoAngle -= stepValue;
        if (servoAngle < 0)
          servoAngle = 0;
      }
      
      myservo.write(servoAngle);
      lcdAngle(servoAngle);
      
      // Save maximum angle
      if (servoAngle > maxAngle)
        maxAngle = servoAngle;
    } else {
      // Rotate servo to maximum angle when angle is less than 3 degrees
      if (maxAngle != 0 && servoAngle != maxAngle) {
        myservo.write(maxAngle);
        lcdAngle(maxAngle);
        delay(1000); // Adjust delay time as needed
        returnToHome = true; // Set flag to return to home position
      }
    }
  } else {
    // Return servo to home position
    if (servoAngle != homePosition) {
      if (servoAngle < homePosition) {
        servoAngle += stepValue;
        if (servoAngle > homePosition)
          servoAngle = homePosition;
      } else {
        servoAngle -= stepValue;
        if (servoAngle < homePosition)
          servoAngle = homePosition;
      }
      
      myservo.write(servoAngle);
      lcdAngle(servoAngle);
    } else {
      returnToHome = false; // Reset flag
      maxAngle = 0; // Reset max angle
    }
  }

  // Check for switch press to return to home position
  if (digitalRead(SW_PIN) == LOW) {
    returnToHome = true;
  }
}

void lcdAngle(int angle) {
  int startChar = 7;
  for (int i = startChar; i < 16; i++) {
    lcd.setCursor(i, 1);
    lcd.print(" ");
  }
  
  lcd.setCursor(startChar, 1);
  lcd.print(angle);
  lcd.print((char)223); // Degree symbol
}
