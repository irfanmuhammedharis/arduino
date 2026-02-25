#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

#define FINGERPRINT_RX 2
#define FINGERPRINT_TX 3

SoftwareSerial mySerial(FINGERPRINT_RX, FINGERPRINT_TX);

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    delay(1); // wait for serial port to connect. Needed for native USB
  }

  Serial.println("Adafruit Fingerprint sensor enrollment test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }
  }
  finger.getParameters();
  Serial.println("Sensor parameters:");
  Serial.print("Status register: 0x"); Serial.println(finger.status_reg, HEX);
  Serial.print("System ID: 0x"); Serial.println(finger.system_id, HEX);
  Serial.print("Capacity: "); Serial.println(finger.capacity);
  Serial.print("Security level: "); Serial.println(finger.security_level);
  Serial.print("Device address: 0x"); Serial.println(finger.device_addr, HEX);
  Serial.print("Packet length: "); Serial.println(finger.packet_len);
  Serial.print("Baud rate: "); Serial.println(finger.baud_rate);

  Serial.println("Ready to scan fingerprint...");
}

void loop() {
  // Wait for a finger to be detected
  while (!finger.getImage()) {
    delay(500);
  }

  // Convert the image to a template
  uint8_t convertResult = finger.image2Tz();
  if (convertResult != FINGERPRINT_OK) {
    Serial.println("Error converting image to template");
    return;
  }

  // Get the template data
  uint8_t templateData[256]; // Assuming template size is 256 bytes
  uint16_t templateLength = finger.getModel(templateData);
  if (templateLength == 0) {
    Serial.println("Error getting template data");
    return;
  }

  // Print the template data in hexadecimal format
  Serial.println("Template data (in hexadecimal):");
  for (int i = 0; i < templateLength; i++) {
    Serial.print(templateData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  delay(1000); // Wait before scanning the next fingerprint
}
