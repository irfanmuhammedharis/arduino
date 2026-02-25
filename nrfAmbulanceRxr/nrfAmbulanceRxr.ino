#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 8); // CE, CSN pins
const byte address[6] = "00001";
const int ledPin = 3; // Connect your LED with a 220-ohm resistor to pin 3
const int red = 4;
void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(red,OUTPUT);
  radio.begin();
  radio.openReadingPipe(1, address);
  radio.setPALevel(RF24_PA_MAX);
  radio.startListening();
  digitalWrite(red, HIGH);
}

void loop() {
  unsigned long startTime = millis();
  int countStrongSignal = 0;
  int totalSamples = 0;

  while (millis() - startTime < 1000) { // Collect data for 1 second
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
      digitalWrite(ledPin, HIGH);
      digitalWrite(red, LOW);
      Serial.println("LED ON: Majority strong signals");
    } else {
      digitalWrite(ledPin, LOW);
      digitalWrite(red, HIGH);
      Serial.println("LED OFF: Majority weak signals");
    }
  } else {
    Serial.println("No signals received in the last second.");
    digitalWrite(ledPin, LOW);
    digitalWrite(red, HIGH);
  }

  // Optional: Add a small delay before the next loop iteration
  delay(10);
}
