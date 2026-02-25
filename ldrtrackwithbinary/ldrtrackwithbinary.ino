#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Pin Definitions
const int LDR1_PIN = A0;
const int LDR2_PIN = A1;
const int LDR3_PIN = A2;
const int LDR4_PIN = A3;
const int SERVO_PIN = 9;
const int BINARY_PIN1 = 10;  // First pin for binary representation
const int BINARY_PIN2 = 11;  // Second pin for binary representation

// Sampling Parameters
const int SAMPLE_INTERVAL = 100;  // 100ms between samples
const int TOTAL_SAMPLES = 5;      // 5 samples total

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
  
  // Set binary output pins as OUTPUT
  pinMode(BINARY_PIN1, OUTPUT);
  pinMode(BINARY_PIN2, OUTPUT);
  
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

// Function to set binary pins based on angle
void setBinaryPins(int angle) {
  switch (angle) {
    case 0:   // 00
      digitalWrite(BINARY_PIN1, LOW);
      digitalWrite(BINARY_PIN2, LOW);
      break;
    case 25:  // 01
      digitalWrite(BINARY_PIN1, LOW);
      digitalWrite(BINARY_PIN2, HIGH);
      break;
    case 50:  // 10
      digitalWrite(BINARY_PIN1, HIGH);
      digitalWrite(BINARY_PIN2, LOW);
      break;
    case 75:  // 11
      digitalWrite(BINARY_PIN1, HIGH);
      digitalWrite(BINARY_PIN2, HIGH);
      break;
    default:
      // Default case (shouldn't happen)
      digitalWrite(BINARY_PIN1, LOW);
      digitalWrite(BINARY_PIN2, LOW);
  }
}

void loop() {
  // Calculate average values for each LDR over sampling period
  int ldr1Value = calculateAverageLDR(LDR1_PIN);
  int ldr2Value = calculateAverageLDR(LDR2_PIN);
  int ldr3Value = calculateAverageLDR(LDR3_PIN);
  int ldr4Value = calculateAverageLDR(LDR4_PIN);
  
  // Determine which LDR has the highest average value
  int maxValue = max(max(ldr1Value, ldr2Value), max(ldr3Value, ldr4Value));
  int servoAngle = 0;
  String activeLDR = "None";
  String binaryValue = "00";
  
  // Determine servo angle based on the highest LDR
  if (maxValue == ldr1Value) {
    servoAngle = 0;
    activeLDR = "LDR1";
    binaryValue = "00";
  }
  else if (maxValue == ldr2Value) {
    servoAngle = 25;
    activeLDR = "LDR2";
    binaryValue = "01";
  }
  else if (maxValue == ldr3Value) {
    servoAngle = 50;
    activeLDR = "LDR3";
    binaryValue = "10";
  }
  else if (maxValue == ldr4Value) {
    servoAngle = 75;
    activeLDR = "LDR4";
    binaryValue = "11";
  }
  
  // Move servo to the calculated angle
  myServo.write(servoAngle);
  
  // Set binary pins based on the angle
  setBinaryPins(servoAngle);
  
  // Update LCD display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Active: " + activeLDR);
  lcd.setCursor(0, 1);
  lcd.print("Angle: " + String(servoAngle) + " (" + binaryValue + ")");
  
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
  Serial.print(servoAngle);
  Serial.print(" Binary: ");
  Serial.println(binaryValue);
}