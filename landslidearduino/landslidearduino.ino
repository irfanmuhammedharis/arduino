/*
 * ADXL335 Accelerometer Change Detection
 * 
 * This code interfaces with an ADXL335 analog accelerometer and:
 * - Reads X, Y, Z values from analog pins
 * - Detects changes from previous stable values
 * - Sends alerts when change is detected
 * - Updates reference values after alert
 */

// Pin definitions for ADXL335
const int xPin = A0;    // X-axis input
const int yPin = A1;    // Y-axis input 
const int zPin = A2;    // Z-axis input
int dit =2;

// Variables to store sensor values
int xValue = 0;
int yValue = 0;
int zValue = 0;

// Reference values (starting point)
int xRef = 0;
int yRef = 0;
int zRef = 0;

// Threshold for change detection (adjust as needed)
const int threshold = 20;  // Sensitivity value (lower = more sensitive)

// Stabilization variables
const int stabilizationTime = 1000;  // Time to wait before setting new reference (ms)
unsigned long lastChangeTime = 0;    // Time of last detected change
bool changePending = false;          // Flag for pending reference update

void setup() {
  Serial.begin(9600);  // Initialize serial communication
  pinMode(dit, OUTPUT);
  // Initial delay to let sensor stabilize
  delay(1000);
  
  // Read initial reference values
  xRef = analogRead(xPin);
  yRef = analogRead(yPin);
  zRef = analogRead(zPin);
  digitalWrite(dit,LOW);
  Serial.println("ADXL335 Change Detection System Initialized");
  Serial.print("Initial reference values - X: ");
  Serial.print(xRef);
  Serial.print(" Y: ");
  Serial.print(yRef);
  Serial.print(" Z: ");
  Serial.println(zRef);
}

void loop() {
  // Read current accelerometer values
  xValue = analogRead(xPin);
  yValue = analogRead(yPin);
  zValue = analogRead(zPin);
  
  // Check if values have changed beyond threshold
  bool changeDetected = false;
  
  if (abs(xValue - xRef) > threshold) {
    changeDetected = true;
  }
  else if (abs(yValue - yRef) > threshold) {
    changeDetected = true;
  }
  else if (abs(zValue - zRef) > threshold) {
    changeDetected = true;
  }
  
  // If change detected and not waiting for stabilization
  if (changeDetected && !changePending) {
    // Alert about the change
    Serial.println("*** CHANGE DETECTED ***");
    digitalWrite(dit,HIGH);
    delay(500);
    digitalWrite(dit,LOW);
    Serial.print("Previous: X=");
    Serial.print(xRef);
    Serial.print(" Y=");
    Serial.print(yRef);
    Serial.print(" Z=");
    Serial.println(zRef);
    
    Serial.print("Current: X=");
    Serial.print(xValue);
    Serial.print(" Y=");
    Serial.print(yValue);
    Serial.print(" Z=");
    Serial.println(zValue);
    
    Serial.print("Delta: X=");
    Serial.print(xValue - xRef);
    Serial.print(" Y=");
    Serial.print(yValue - yRef);
    Serial.print(" Z=");
    Serial.println(zValue - zRef);
    
    // Start the stabilization timer
    lastChangeTime = millis();
    changePending = true;
  }
  
  // Check if it's time to update reference values after a change
  if (changePending && (millis() - lastChangeTime > stabilizationTime)) {
    // Update reference values
    xRef = analogRead(xPin);
    yRef = analogRead(yPin);
    zRef = analogRead(zPin);
    
    Serial.println("New reference values set");
    Serial.print("X: ");
    Serial.print(xRef);
    Serial.print(" Y: ");
    Serial.print(yRef);
    Serial.print(" Z: ");
    Serial.println(zRef);
    
    changePending = false;  // Reset the flag
  }
  
  // Short delay between readings
  delay(100);
}