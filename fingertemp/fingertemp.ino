#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Accesspoint";
const char* password = "esp12345";
const char* serverUrl = "http://192.168.137.1/vote/api.php";
#include <SoftwareSerial.h>
#include <HardwareSerial.h>
//#define FINGERPRINT_RX 2
//#define FINGERPRINT_TX 3
#define FINGERPRINT_PASSWORD 12345
HardwareSerial mySerial(2);

//SoftwareSerial mySerial(FINGERPRINT_RX, FINGERPRINT_TX);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
String sensorValue = "";
String hexValue = "";
String templateData;
byte fingerprintTemplate[256];



void setup() {
  Serial.begin(9600);
  while (!Serial)
    delay(10);  // For Leonardo/Micro

  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1)
      ;
  }
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
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
    // Serial.print(finger.getRecvPacket(i), HEX);
     fingerprintTemplate[i] = finger.getRecvPacket(i);
  }
  String fingerprintString = byteArrayToString(fingerprintTemplate, sizeof(fingerprintTemplate));
  Serial.println("Fingerprint Template: " + fingerprintString);

  
  dataSend(fingerprintString);
  Serial.println();
  Serial.println("Remove finger...");
  while (finger.getImage() != FINGERPRINT_NOFINGER)
    ;
  delay(1000);  // Wait a moment before trying again
}


String byteArrayToString(byte byteArray[], int length) {
  String result = "";
  for (int i = 0; i < length; i++) {
    // Convert each byte to a two-digit hexadecimal string
    String hexString = String(byteArray[i], HEX);
    // Pad with zero if necessary to ensure each byte is represented by two characters
    if (hexString.length() == 1) {
      hexString = "0" + hexString;
    }
    result += hexString;
  }
  return result;
}

void dataSend(String sensorValues) {
  String formData = "new_data=" + sensorValues;

  Serial.println(formData);
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(formData);
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error sending request: ");
    Serial.println(httpResponseCode);
  }

  http.end();

  delay(1000);
}