#include <SoftwareSerial.h>

// Configure SIM800L module pinouts
#define SIM800_TX_PIN 4
#define SIM800_RX_PIN 5
#define SIM800_PWR_PIN 16

// Create a SoftwareSerial object for SIM800L module communication
SoftwareSerial sim800l(SIM800_TX_PIN, SIM800_RX_PIN);

void setup() {
  // Set baud rate to 9600
  Serial.begin(9600);
  sim800l.begin(9600);

  // Power on the SIM800L module
  pinMode(SIM800_PWR_PIN, OUTPUT);
  digitalWrite(SIM800_PWR_PIN, HIGH);
  delay(1000);
  digitalWrite(SIM800_PWR_PIN, LOW);
  delay(1000);
  digitalWrite(SIM800_PWR_PIN, HIGH);
  delay(2000);
}

void loop() {
  // Dial a phone number
  sim800l.println("ATD+919633118080;"); // Replace with the phone number you want to call
  delay(5000);

  // Hang up the call
  sim800l.println("ATH");
  delay(1000);
}
