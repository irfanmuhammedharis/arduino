#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

#define FINGERPRINT_RX 2
#define FINGERPRINT_TX 3
#define FINGERPRINT_PASSWORD 12345

SoftwareSerial mySerial(FINGERPRINT_RX, FINGERPRINT_TX);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  Serial.begin(9600);
  while (!Serial)
    delay(10); // For Leonardo/Micro

  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1)
      ;
  }
}

void loop() {
  uint8_t p = finger.getImage();
  switch (p) {
  case FINGERPRINT_OK:
    Serial.println("Image taken");
    break;
  case FINGERPRINT_NOFINGER:
    Serial.println("No finger detected");
    return;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return;
  case FINGERPRINT_IMAGEFAIL:
    Serial.println("Imaging error");
    return;
  default:
    Serial.println("Unknown error");
    return;
  }

  p = finger.image2Tz();
  switch (p) {
  case FINGERPRINT_OK:
    Serial.println("Image converted");
    break;
  case FINGERPRINT_IMAGEMESS:
    Serial.println("Image too messy");
    return;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return;
  case FINGERPRINT_FEATUREFAIL:
    Serial.println("Could not find fingerprint features");
    return;
  case FINGERPRINT_INVALIDIMAGE:
    Serial.println("Invalid image");
    return;
  default:
    Serial.println("Unknown error");
    return;
  }

  Serial.print("Template data: ");
  for (int i = 0; i < 256; i++) {
    Serial.print(finger.getRecvPacket(i), HEX);
  }
  Serial.println();

  // Wait for finger to be removed
  Serial.println("Remove finger...");
  while (finger.getImage() != FINGERPRINT_NOFINGER);
 delay(1000); // Wait a moment before trying again
}
