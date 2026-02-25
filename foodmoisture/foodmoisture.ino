// Define pins for the sensor, potentiometer, and relays
#define MOISTURE_SENSOR_PIN A0   // Analog pin for the moisture sensor
#define POT_PIN A1               // Analog pin for the 10k potentiometer
#define RELAY1_PIN 7             // Digital pin for Relay 1
#define RELAY2_PIN 8             // Digital pin for Relay 2

void setup() {
  // Initialize Serial communication for debugging outputs
  Serial.begin(9600);
  
  // Set up sensor and potentiometer pins
  pinMode(MOISTURE_SENSOR_PIN, INPUT);
  pinMode(POT_PIN, INPUT);
  
  // Set up relay pins as OUTPUT
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  
  // Initialize relays to OFF state (assuming LOW turns them off)
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
}

void loop() {
  // Read the moisture sensor value (0-1023)
  int moistureValue = analogRead(MOISTURE_SENSOR_PIN);
  
  // Read the 10k potentiometer value (0-1023)
  int potValue = analogRead(POT_PIN);
  
  // Map the potentiometer value to a threshold range for the moisture sensor
  int threshold = map(potValue, 0, 600, 0, 2000);

  // Print detailed serial output for debugging and monitoring
  Serial.print("Moisture Value: ");
  Serial.print(moistureValue);
  Serial.print(" | Pot Value: ");
  Serial.print(potValue);
  Serial.print(" | Mapped Threshold: ");
  Serial.println(threshold);

  // Check if the moisture value is lower than the threshold
  if (moistureValue < threshold) {
    // Activate both relays (assumed to be active HIGH)
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, HIGH);
    Serial.println("Alert: Moisture is below threshold. Both relays activated.");
  } else {
    // Deactivate relays if moisture level is sufficient
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    Serial.println("Status: Moisture is sufficient. Relays deactivated.");
  }

  // Wait for 1 second before taking the next reading
  delay(1000);
}
