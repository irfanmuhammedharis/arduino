const int metalDetectorPin = 2; // Replace with the pin you connected the sensor to

void setup() {
  Serial.begin(9600); // Initialize serial communication for debugging (optional)
  pinMode(metalDetectorPin, INPUT); // Set the metal detector pin as input
}

void loop() {
  int metalDetected = digitalRead(metalDetectorPin);

  if (metalDetected == HIGH) {
    Serial.println("Metal detected!");
  } else {
    Serial.println("No metal detected.");
  }

   // Delay between readings (optional)
}
