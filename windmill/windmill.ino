// Pin definitions
const int irSensorPin = 2;    // IR sensor signal pin
const int motorPin1 = 3;      // Motor driver IN1
const int motorPin2 = 4;      // Motor driver IN2

void setup() {
  // Set up pins
  pinMode(irSensorPin, INPUT);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);

  // Initialize motor to off
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);

  // Serial for debugging (optional)
  Serial.begin(9600);
}

void loop() {
  // Read IR sensor
  int irState = digitalRead(irSensorPin);

  // If IR sensor is active
  if (irState == LOW) {
    Serial.println("IR sensor activated! Rotating motor...");
    
    // Rotate motor
    digitalWrite(motorPin1, HIGH);
    digitalWrite(motorPin2, LOW);

    // Wait for 5 seconds
    delay(5000);

    // Stop motor
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
    Serial.println("Motor stopped.");
  }

  // Small delay to prevent excessive polling
  delay(100);
}
