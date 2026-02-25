/*
 * Solar Tracking Panel System - Direct Angle Tracking
 * Author: irfanmuhammedharis
 * Date: 2025-07-29 05:09:41
 * 
 * SIMPLE LOGIC: Servo moves to the angle of the LDR with highest reading
 * 
 * LDR POSITIONING:
 * LDR1 (A0): 30° position
 * LDR2 (A1): 60° position  
 * LDR3 (A2): 90° position
 * LDR4 (A3): 120° position
 */

#include <Servo.h>

// === CONFIGURATION ===
#define SERVO_PIN 9
#define LDR1_PIN A0    // 30° position
#define LDR2_PIN A1    // 60° position  
#define LDR3_PIN A2    // 90° position
#define LDR4_PIN A3    // 120° position                                                                                                 

// LDR angles corresponding to each sensor
const int LDR_ANGLES[4] = {30, 50, 70, 90};

// Servo settings
#define SERVO_MIN 0
#define SERVO_MAX 180
#define SERVO_CENTER 90

// Tracking settings
#define MIN_LIGHT_DIFFERENCE 15  // Minimum difference to trigger movement
#define MOVEMENT_DELAY 1000      // Delay between movements (ms)
#define SAMPLE_COUNT 3           // Number of readings to average

// === GLOBAL VARIABLES ===
Servo solarServo;
int currentPosition = SERVO_CENTER;
unsigned long lastMovement = 0;

void setup() {
  Serial.begin(9600);
  delay(100);
  
  Serial.println(F("=== DIRECT ANGLE SOLAR TRACKER ==="));
  Serial.println(F("Logic: Servo moves to angle of brightest LDR"));
  Serial.println(F(""));
  Serial.println(F("LDR Positions:"));
  Serial.println(F("LDR1 (A0): 30°"));
  Serial.println(F("LDR2 (A1): 60°"));
  Serial.println(F("LDR3 (A2): 90°"));
  Serial.println(F("LDR4 (A3): 120°"));
  Serial.println(F("====================================="));
  
  // Initialize servo
  solarServo.attach(SERVO_PIN);
  solarServo.write(SERVO_CENTER);
  delay(1000);
  
  Serial.println(F("System ready - Starting tracking..."));
  Serial.println(F("Format: L1(30°)|L2(60°)|L3(90°)|L4(120°)|Brightest|Target|Current"));
  Serial.println(F("----------------------------------------------------------------"));
}

void loop() {
  // Read all LDR sensors
  int ldrReadings[4] = {0, 0, 0, 0};
  
  // Take multiple samples for stability
  for (int sample = 0; sample < SAMPLE_COUNT; sample++) {
    ldrReadings[0] += analogRead(LDR1_PIN);
    ldrReadings[1] += analogRead(LDR2_PIN);
    ldrReadings[2] += analogRead(LDR3_PIN);
    ldrReadings[3] += analogRead(LDR4_PIN);
    delay(10);
  }
  
  // Calculate averages and convert to light percentage
  int lightLevels[4];
  for (int i = 0; i < 4; i++) {
    int avgReading = ldrReadings[i] / SAMPLE_COUNT;
    lightLevels[i] = map(avgReading, 0, 1023, 100, 0); // Invert LDR reading
    lightLevels[i] = constrain(lightLevels[i], 0, 100);
  }
  
  // Find the LDR with highest reading
  int brightestLDR = findBrightestLDR(lightLevels);
  int brightestAngle = LDR_ANGLES[brightestLDR];
  int maxLightLevel = lightLevels[brightestLDR];
  
  // Print current readings
  printReadings(lightLevels, brightestLDR, brightestAngle);
  
  // Check if we should move the servo
  if (shouldMove(maxLightLevel, brightestAngle)) {
    moveServoToAngle(brightestAngle);
    lastMovement = millis();
    
    Serial.print(F(" -> MOVED to "));
    Serial.print(currentPosition);
    Serial.println(F("°"));
  } else {
    Serial.println(F(" -> No movement"));
  }
  
  delay(500); // Main loop delay
}

int findBrightestLDR(int lightLevels[4]) {
  int brightestIndex = 0;
  int maxLight = lightLevels[0];
  
  for (int i = 1; i < 4; i++) {
    if (lightLevels[i] > maxLight) {
      maxLight = lightLevels[i];
      brightestIndex = i;
    }
  }
  
  return brightestIndex;
}

bool shouldMove(int maxLightLevel, int targetAngle) {
  // Check if enough time has passed since last movement
  if ((millis() - lastMovement) < MOVEMENT_DELAY) {
    return false;
  }
  
  // Check if there's enough light to track
  if (maxLightLevel < 20) {
    return false;
  }
  
  // Check if target angle is significantly different from current position
  int angleDifference = abs(targetAngle - currentPosition);
  if (angleDifference < MIN_LIGHT_DIFFERENCE) {
    return false;
  }
  
  return true;
}

void moveServoToAngle(int targetAngle) {
  // Constrain target angle to servo limits
  targetAngle = constrain(targetAngle, SERVO_MIN, SERVO_MAX);
  
  // Calculate movement direction and distance
  int distance = abs(targetAngle - currentPosition);
  int direction = (targetAngle > currentPosition) ? 1 : -1;
  
  // Move servo smoothly to target position
  for (int i = 0; i < distance; i++) {
    currentPosition += direction;
    solarServo.write(currentPosition);
    delay(20); // Smooth movement delay
  }
}

void printReadings(int lightLevels[4], int brightestLDR, int brightestAngle) {
  // Print light levels for each LDR
  for (int i = 0; i < 4; i++) {
    Serial.print(lightLevels[i]);
    if (i < 3) Serial.print(F("|"));
  }
  
  // Print brightest LDR info
  Serial.print(F("|LDR"));
  Serial.print(brightestLDR + 1);
  Serial.print(F("("));
  Serial.print(LDR_ANGLES[brightestLDR]);
  Serial.print(F("°)|"));
  
  // Print target and current positions
  Serial.print(brightestAngle);
  Serial.print(F("°|"));
  Serial.print(currentPosition);
  Serial.print(F("°"));
}