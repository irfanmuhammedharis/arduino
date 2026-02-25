#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Pin Definitions
const int LDR1_PIN = A0;
const int LDR2_PIN = A1;
const int LDR3_PIN = A2;
const int LDR4_PIN = A3;
const int SERVO_PIN = 9;

// Sampling Parameters
const int SAMPLE_INTERVAL = 100;  // 100ms between samples
const int TOTAL_SAMPLES = 5;     // 30 * 100ms = 3 seconds of sampling

// Create servo and LCD objects
Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD address to 0x27 for a 16 chars and 2 line display

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // Attach servo to its pin
  myServo.attach(SERVO_PIN);
  
  // Initial message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LDR Servo");
  lcd.setCursor(0, 1);
  lcd.print("Control System");
  delay(2000);
}

// Function to calculate average of LDR readings
int calculateAverageLDR(int pin) {
  long total = 0;
  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    total += analogRead(pin);
    delay(SAMPLE_INTERVAL);
  }
  return total / TOTAL_SAMPLES;
}

void loop() {
  // Calculate average values for each LDR over 3 seconds
  int ldr1Value = calculateAverageLDR(LDR1_PIN);
  int ldr2Value = calculateAverageLDR(LDR2_PIN);
  int ldr3Value = calculateAverageLDR(LDR3_PIN);
  int ldr4Value = calculateAverageLDR(LDR4_PIN);
  
  // Determine which LDR has the highest average value
  int maxValue = max(max(ldr1Value, ldr2Value), max(ldr3Value, ldr4Value));
  int servoAngle = 0;
  String activeLDR = "None";
  
  // Determine servo angle based on the highest LDR
  if (maxValue == ldr1Value) {
    servoAngle = 0;
    activeLDR = "LDR1";
  }
  else if (maxValue == ldr2Value) {
    servoAngle = 25;
    activeLDR = "LDR2";
  }
  else if (maxValue == ldr3Value) {
    servoAngle = 50;
    activeLDR = "LDR3";
  }
  else if (maxValue == ldr4Value) {
    servoAngle = 75;
    activeLDR = "LDR4";
  }
  
  // Move servo to the calculated angle
  myServo.write(servoAngle);
  
  // Update LCD display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Active: " + activeLDR);
  lcd.setCursor(0, 1);
  lcd.print("Angle: " + String(servoAngle) + " deg");
  
  // Print values to serial for debugging
  Serial.print("LDR1 Avg: ");
  Serial.print(ldr1Value);
  Serial.print(" LDR2 Avg: ");
  Serial.print(ldr2Value);
  Serial.print(" LDR3 Avg: ");
  Serial.print(ldr3Value);
  Serial.print(" LDR4 Avg: ");
  Serial.print(ldr4Value);
  Serial.print(" Servo Angle: ");
  Serial.println(servoAngle);
}