/*******************************************************************************
 * File: RC_Car_SafeStop.ino
 * Version: 4.0 SIMPLIFIED
 * Board: Arduino Uno R3
 * Date: 2024-11-11
 * 
 * Description: Bluetooth RC car with 30cm safety stop
 *              Simplified for reliability with improved ultrasonic handling
 * 
 * Key Features:
 * - Stops automatically when obstacle detected at ≤30cm
 * - Simple Bluetooth control (F/B/L/R/S)
 * - Filtered ultrasonic readings for reliability
 * - Clear debug output via Serial Monitor
 * 
 * Based on Arduino Examples:
 * - Examples > 04.Communication > SerialEvent
 * - Examples > 06.Sensors > Ping
 * - Examples > 03.Analog > Fading
 * 
 * Safety: HC-05 RX protected with voltage divider (5V→3.3V)
 ******************************************************************************/

// ============================================================================
// PIN DEFINITIONS
// ============================================================================
// Motor pins
#define LEFT_MOTOR_SPEED   3   // PWM
#define LEFT_MOTOR_DIR1    4   
#define LEFT_MOTOR_DIR2    5   
#define RIGHT_MOTOR_DIR1   6   
#define RIGHT_MOTOR_DIR2   7   
#define RIGHT_MOTOR_SPEED  9   // PWM

// Sensor pins
#define TRIG_PIN          2    
#define ECHO_PIN          12   

// LED pins
#define RED_LED           10   // Obstacle warning
#define GREEN_LED         11   // System ready

// ============================================================================
// CONFIGURATION
// ============================================================================
#define STOP_DISTANCE     30   // Stop if object closer than 30cm
#define MOTOR_SPEED       180  // Normal speed (0-255)
#define TURN_SPEED        150  // Turning speed
#define SENSOR_SAMPLES    3    // Number of readings to average

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
char command = 'S';            // Current command
long distance = 100;           // Distance to obstacle (cm)
unsigned long lastPrint = 0;   // For debug output timing
boolean obstacleDetected = false;

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  // Initialize serial for Bluetooth
  Serial.begin(9600);
  
  // Print startup message
  Serial.println(F("============================"));
  Serial.println(F("   RC CAR SAFETY SYSTEM"));
  Serial.println(F("============================"));
  Serial.println(F("Commands:"));
  Serial.println(F("  F = Forward"));
  Serial.println(F("  B = Backward"));
  Serial.println(F("  L = Left"));
  Serial.println(F("  R = Right"));
  Serial.println(F("  S = Stop"));
  Serial.println(F(""));
  Serial.println(F("Safety: Auto-stop at 30cm"));
  Serial.println(F("============================"));
  
  // Configure pins
  pinMode(LEFT_MOTOR_SPEED, OUTPUT);
  pinMode(LEFT_MOTOR_DIR1, OUTPUT);
  pinMode(LEFT_MOTOR_DIR2, OUTPUT);
  pinMode(RIGHT_MOTOR_DIR1, OUTPUT);
  pinMode(RIGHT_MOTOR_DIR2, OUTPUT);
  pinMode(RIGHT_MOTOR_SPEED, OUTPUT);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  
  // Initial state
  stopMotors();
  digitalWrite(TRIG_PIN, LOW);
  
  // Startup indication
  for(int i = 0; i < 5; i++) {
    digitalWrite(GREEN_LED, HIGH);
    delay(100);
    digitalWrite(GREEN_LED, LOW);
    delay(100);
  }
  digitalWrite(GREEN_LED, HIGH);
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
  // Read distance with averaging for reliability
  distance = readDistanceAverage();
  
  // Check for obstacle
  obstacleDetected = (distance <= STOP_DISTANCE && distance > 0);
  
  // Update warning LED
  digitalWrite(RED_LED, obstacleDetected);
  
  // Print status every 500ms
  if (millis() - lastPrint > 500) {
    Serial.print(F("Distance: "));
    Serial.print(distance);
    Serial.print(F(" cm"));
    if (obstacleDetected) {
      Serial.print(F(" [OBSTACLE!]"));
    }
    Serial.print(F(" | Command: "));
    Serial.println(command);
    lastPrint = millis();
  }
  
  // Process Bluetooth commands
  if (Serial.available() > 0) {
    char newCommand = Serial.read();
    newCommand = toupper(newCommand);
    
    // Process command
    if (newCommand == 'F' || newCommand == 'B' || 
        newCommand == 'L' || newCommand == 'R' || 
        newCommand == 'S') {
      command = newCommand;
      Serial.print(F(">> Command received: "));
      Serial.println(command);
    }
  }
  
  // Execute movement with safety check
  executeCommand();
  
  // Small delay for stability
  delay(50);
}

// ============================================================================
// ULTRASONIC FUNCTIONS
// ============================================================================
long readDistance() {
  // Clear trigger
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Send trigger pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read echo with timeout
  long duration = pulseIn(ECHO_PIN, HIGH, 20000); // 20ms timeout
  
  // Convert to cm
  long cm = duration / 58; // Simplified formula: duration / 58 = cm
  
  // Validate reading
  if (cm == 0 || cm > 200) {
    return 200; // Return max range if invalid
  }
  
  return cm;
}

long readDistanceAverage() {
  // Take multiple readings and average for reliability
  long sum = 0;
  int validReadings = 0;
  
  for (int i = 0; i < SENSOR_SAMPLES; i++) {
    long reading = readDistance();
    if (reading > 0 && reading < 200) {
      sum += reading;
      validReadings++;
    }
    delay(10); // Short delay between readings
  }
  
  if (validReadings > 0) {
    return sum / validReadings;
  } else {
    return 200; // Default to safe distance
  }
}

// ============================================================================
// COMMAND EXECUTION
// ============================================================================
void executeCommand() {
  // SAFETY: Stop if obstacle detected during forward movement
  if (command == 'F' && obstacleDetected) {
    stopMotors();
    Serial.println(F("!! SAFETY STOP - Obstacle at 30cm or less !!"));
    command = 'S';
    
    // Flash warning
    for(int i = 0; i < 3; i++) {
      digitalWrite(RED_LED, LOW);
      delay(100);
      digitalWrite(RED_LED, HIGH);
      delay(100);
    }
    return;
  }
  
  // Execute command
  switch(command) {
    case 'F':
      moveForward();
      break;
      
    case 'B':
      moveBackward();
      break;
      
    case 'L':
      turnLeft();
      break;
      
    case 'R':
      turnRight();
      break;
      
    case 'S':
    default:
      stopMotors();
      break;
  }
}

// ============================================================================
// MOTOR CONTROL FUNCTIONS
// ============================================================================
void moveForward() {
  // Check distance again before moving
  if (obstacleDetected) {
    stopMotors();
    return;
  }
  
  // Left motor forward
  digitalWrite(LEFT_MOTOR_DIR1, HIGH);
  digitalWrite(LEFT_MOTOR_DIR2, LOW);
  analogWrite(LEFT_MOTOR_SPEED, MOTOR_SPEED);
  
  // Right motor forward
  digitalWrite(RIGHT_MOTOR_DIR1, HIGH);
  digitalWrite(RIGHT_MOTOR_DIR2, LOW);
  analogWrite(RIGHT_MOTOR_SPEED, MOTOR_SPEED);
}

void moveBackward() {
  // Left motor backward
  digitalWrite(LEFT_MOTOR_DIR1, LOW);
  digitalWrite(LEFT_MOTOR_DIR2, HIGH);
  analogWrite(LEFT_MOTOR_SPEED, MOTOR_SPEED);
  
  // Right motor backward
  digitalWrite(RIGHT_MOTOR_DIR1, LOW);
  digitalWrite(RIGHT_MOTOR_DIR2, HIGH);
  analogWrite(RIGHT_MOTOR_SPEED, MOTOR_SPEED);
}

void turnLeft() {
  // Left motor slow/stop
  digitalWrite(LEFT_MOTOR_DIR1, HIGH);
  digitalWrite(LEFT_MOTOR_DIR2, LOW);
  analogWrite(LEFT_MOTOR_SPEED, TURN_SPEED / 2);
  
  // Right motor normal
  digitalWrite(RIGHT_MOTOR_DIR1, HIGH);
  digitalWrite(RIGHT_MOTOR_DIR2, LOW);
  analogWrite(RIGHT_MOTOR_SPEED, TURN_SPEED);
}

void turnRight() {
  // Left motor normal
  digitalWrite(LEFT_MOTOR_DIR1, HIGH);
  digitalWrite(LEFT_MOTOR_DIR2, LOW);
  analogWrite(LEFT_MOTOR_SPEED, TURN_SPEED);
  
  // Right motor slow/stop
  digitalWrite(RIGHT_MOTOR_DIR1, HIGH);
  digitalWrite(RIGHT_MOTOR_DIR2, LOW);
  analogWrite(RIGHT_MOTOR_SPEED, TURN_SPEED / 2);
}

void stopMotors() {
  // Stop both motors completely
  digitalWrite(LEFT_MOTOR_DIR1, LOW);
  digitalWrite(LEFT_MOTOR_DIR2, LOW);
  analogWrite(LEFT_MOTOR_SPEED, 0);
  
  digitalWrite(RIGHT_MOTOR_DIR1, LOW);
  digitalWrite(RIGHT_MOTOR_DIR2, LOW);
  analogWrite(RIGHT_MOTOR_SPEED, 0);
}

// ============================================================================
// End of RC_Car_SafeStop.ino
// ============================================================================