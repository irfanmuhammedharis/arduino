#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
WiFiClient client;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); 
  Serial.begin(9600);
  WiFi.mode(WIFI_STA); // set WiFi mode to station
  WiFi.begin("myAP", "password"); // connect to access point with SSID and password
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
}

void loop() {
 while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}
