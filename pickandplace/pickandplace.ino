

#include <Servo.h>

// Define servo pins
#define SERVO_X_PIN 9
#define SERVO_Y_PIN 10
#define SERVO_Z_PIN 11

// Define servo objects
Servo servoX;  // Base rotation (X-axis)
Servo servoY;  // Shoulder pitch (Y-axis)
Servo servoZ;  // Elbow pitch (Z-axis)

// Define servo angle limits (0-180 degrees)
#define MIN_ANGLE 0
#define MAX_ANGLE 180

// Serial buffer
String inputString = "";         // String to hold incoming data
bool stringComplete = false;     // Whether the string is complete

// Function prototypes
void parseAndMove(String command);
bool validateAngle(int angle);
void moveServo(Servo &servo, int angle);

void setup() {
  // Initialize serial communication at 9600 baud
  Serial.begin(9600);
  
  // Reserve 200 bytes for the inputString
  inputString.reserve(200);
  
  // Attach servos to pins
  servoX.attach(SERVO_X_PIN);
  servoY.attach(SERVO_Y_PIN);
  servoZ.attach(SERVO_Z_PIN);
  
  // Initialize servos to neutral position (90 degrees)
  servoX.write(90);
  servoY.write(90);
  servoZ.write(90);
  
  Serial.println("Servo Arm Controller Initialized.");
  Serial.println("Send commands like: X90 Y45 Z120");
}

void loop() {
  // Check for completed serial string
  if (stringComplete) {
    parseAndMove(inputString);
    inputString = "";
    stringComplete = false;
  }
}

// Serial event handler (based on SerialEvent example)
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    
    // If newline, set stringComplete
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

// Parse command and move servos
void parseAndMove(String command) {
  // Expected format: "X<angle> Y<angle> Z<angle>"
  int xPos = -1, yPos = -1, zPos = -1;
  
  // Find positions of X, Y, Z
  int xIndex = command.indexOf('X');
  int yIndex = command.indexOf('Y');
  int zIndex = command.indexOf('Z');
  
  if (xIndex != -1) {
    xPos = command.substring(xIndex + 1, yIndex != -1 ? yIndex : command.length()).toInt();
  }
  if (yIndex != -1) {
    yPos = command.substring(yIndex + 1, zIndex != -1 ? zIndex : command.length()).toInt();
  }
  if (zIndex != -1) {
    zPos = command.substring(zIndex + 1).toInt();
  }
  
  // Validate and move
  if (xPos != -1 && validateAngle(xPos)) {
    moveServo(servoX, xPos);
    Serial.print("X moved to: ");
    Serial.println(xPos);
  } else if (xPos != -1) {
    Serial.println("Invalid X angle (0-180)");
  }
  
  if (yPos != -1 && validateAngle(yPos)) {
    moveServo(servoY, yPos);
    Serial.print("Y moved to: ");
    Serial.println(yPos);
  } else if (yPos != -1) {
    Serial.println("Invalid Y angle (0-180)");
  }
  
  if (zPos != -1 && validateAngle(zPos)) {
    moveServo(servoZ, zPos);
    Serial.print("Z moved to: ");
    Serial.println(zPos);
  } else if (zPos != -1) {
    Serial.println("Invalid Z angle (0-180)");
  }
  
  Serial.println("Command processed.");
}

// Validate angle
bool validateAngle(int angle) {
  return (angle >= MIN_ANGLE && angle <= MAX_ANGLE);
}

// Move servo with delay (non-blocking in spirit, but uses delay for simplicity; for full non-blocking, use millis)
void moveServo(Servo &servo, int angle) {
  servo.write(angle);
  delay(15);  // Small delay to allow servo to reach position (adjust as needed)
}