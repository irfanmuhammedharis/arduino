#include <WiFi.h> // Use <WiFi.h> for ESP32; use <ESP8266WiFi.h> for ESP8266

void setup() {
  Serial.begin(115200);
  
  // Set WiFi mode to Station to access the MAC address
  WiFi.mode(WIFI_STA);
  delay(1000);
  
  // Get the MAC address
  String macAddress = WiFi.macAddress();
  
  Serial.print("MAC Address: ");
  Serial.println(macAddress);
}

void loop() {
  // Nothing needed here
}
