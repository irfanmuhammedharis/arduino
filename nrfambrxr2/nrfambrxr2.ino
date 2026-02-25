#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 8); // CE, CSN pins
const byte address[6] = "00001";

// Original LEDs
const int ledPin = 3; // Green LED 1
const int red = 4;    // Red LED 1

// New LEDs for Traffic Signal
const int ledPin2 = 5; // Green LED 2 (inverse of ledPin)
const int red2 = 6;    // Red LED 2 (inverse of red)

// State variable for traffic signal
bool trafficGreen = true; // true = Green State, false = Red State
unsigned long lastSwitchTime = 0; // Last time the state switched
bool previousTrafficGreen = true; // To track changes in state

void setup() {
  Serial.begin(9600);

  // Set up pins for LEDs
  pinMode(ledPin, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(red2, OUTPUT);

  // Initialize radio
  radio.begin();
  radio.openReadingPipe(1, address);
  radio.setPALevel(RF24_PA_MAX);
  radio.startListening();

  // Initial LED state
  digitalWrite(red, HIGH);      // Red ON (default state)
  digitalWrite(ledPin2, HIGH); // Green LED 2 ON (inverse of Green LED 1)
}

void loop() {
  unsigned long currentTime = millis();
  int countStrongSignal = 0;
  int totalSamples = 0;

  // Check for nRF24 signals for 1 second
  unsigned long startTime = millis();
  while (millis() - startTime < 1000) {
    if (radio.available()) {
      char text[32] = {0};
      radio.read(&text, sizeof(text));

      // Check the RPD status
      if (radio.testRPD()) {
        countStrongSignal++;
      }
      totalSamples++;
    }
  }

  if (totalSamples > 0) {
    float strongSignalPercentage = (float)countStrongSignal / totalSamples;

    // Determine if the percentage of strong signals is above a threshold
    if (strongSignalPercentage > 0.5) { // Adjust threshold as needed
      digitalWrite(ledPin, HIGH); // Green LED 1 ON
      digitalWrite(red, LOW);     // Red LED 1 OFF

      digitalWrite(ledPin2, LOW); // Green LED 2 OFF (inverse of Green LED 1)
      digitalWrite(red2, HIGH);   // Red LED 2 ON (inverse of Red LED 1)

      Serial.println("LED ON: Majority strong signals");
    } else {
      digitalWrite(ledPin, LOW);  // Green LED 1 OFF
      digitalWrite(red, HIGH);   // Red LED 1 ON

      digitalWrite(ledPin2, HIGH); // Green LED 2 ON (inverse of Green LED 1)
      digitalWrite(red2, LOW);     // Red LED 2 OFF (inverse of Red LED 1)

      Serial.println("LED OFF: Majority weak signals");
    }
  } else {
    // No signals received, activate traffic signal behavior
    if (currentTime - lastSwitchTime >= 10000) { // Switch every 10 seconds
      lastSwitchTime = currentTime;
      trafficGreen = !trafficGreen; // Toggle state

      // Detect state change and print message
      if (trafficGreen != previousTrafficGreen) {
        if (trafficGreen) {
          Serial.println("Traffic Signal Changed: GREEN");
        } else {
          Serial.println("Traffic Signal Changed: RED");
        }
        previousTrafficGreen = trafficGreen; // Update the previous state
      }
    }

    // Update LED states based on trafficGreen
    if (trafficGreen) {
      // Green State
      digitalWrite(ledPin, HIGH); // Green LED 1 ON
      digitalWrite(red, LOW);     // Red LED 1 OFF
      digitalWrite(ledPin2, LOW); // Green LED 2 OFF (inverse of Green LED 1)
      digitalWrite(red2, HIGH);   // Red LED 2 ON (inverse of Red LED 1)
    } else {
      // Red State
      digitalWrite(ledPin, LOW);  // Green LED 1 OFF
      digitalWrite(red, HIGH);    // Red LED 1 ON
      digitalWrite(ledPin2, HIGH);// Green LED 2 ON (inverse of Green LED 1)
      digitalWrite(red2, LOW);    // Red LED 2 OFF (inverse of Red LED 1)
    }
  }

  delay(10);
}
