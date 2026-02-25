#include <Arduino.h>
#include <SoftwareSerial.h>

// Define RX and TX pins for SoftwareSerial
// Recommended to choose pins that are not used for other crucial tasks
int RX_PIN = D5; // GPIO14
int TX_PIN = D6; // GPIO12, not used in this example but required to define SoftwareSerial

SoftwareSerial softSerial(RX_PIN, TX_PIN); // RX, TX

// Structure to hold the incoming data
struct struct_message {
  int id;
  float t;
  float h;
} receivedData;

// Function to parse incoming serial data
bool parseSerialData(String data, struct_message &msg) {
  int firstCommaIndex = data.indexOf(',');
  int secondCommaIndex = data.indexOf(',', firstCommaIndex + 1);

  if (firstCommaIndex == -1 || secondCommaIndex == -1) {
    return false; // Incorrect data format
  }

  msg.id = data.substring(0, firstCommaIndex).toInt();
  msg.t = data.substring(firstCommaIndex + 1, secondCommaIndex).toFloat();
  msg.h = data.substring(secondCommaIndex + 1).toFloat();

  return true;
}

void setup() {
  // Initialize the built-in Serial port for debugging
  Serial.begin(115200);
  Serial.println("ESP8266 Debug Serial Ready");

  // Initialize SoftwareSerial on specified pins
  softSerial.begin(9600); // Set the baud rate to match the ESP32's transmission rate
  Serial.println("ESP8266 SoftwareSerial Ready to receive data at 9600 baud");
}

void loop() {
  // Check if data is available on SoftwareSerial port
  if (softSerial.available() > 0) {
    // Read the incoming data until newline
    String received = softSerial.readStringUntil('\n'); 
    if (parseSerialData(received, receivedData)) {
      // If data is parsed successfully, print it to debug serial
      Serial.print("Received ID: ");
      Serial.println(receivedData.id);
      Serial.print("Temperature: ");
      Serial.println(receivedData.t);
      Serial.print("Humidity: ");
      Serial.println(receivedData.h);
      Serial.println();
    } else {
      // If parsing fails, print an error message to debug serial
      Serial.println("Error parsing received data");
    }
  }
}
