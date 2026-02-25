
#include <ESP8266WiFi.h>

String apiKey = "CQ4JO5DMQMKHF7TT";
const char* ssid = "sreelekshmi";       // WiFi Network's SSID
const char* pass = "sree1234";         // WiFi Network's Password
const char* server = "api.thingspeak.com";

int analogInPin = A0;                  
int bat_percentage;
float base_calibration = 0.0;          

WiFiClient client;

void setup() {
  Serial.begin(115200);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print("*");
  }
  Serial.println("\nWiFi connected");
  Serial.println("Battery Monitoring Initialized");
}

void loop() {
  float voltage = getAverageVoltage(10);   // Averages 10 samples for stability
  voltage = calibratedVoltage(voltage);    // Apply refined calibration

  bat_percentage = mapfloat(voltage, 2.8, 4.2, 0, 100);
  bat_percentage = constrain(bat_percentage, 1, 100);

  Serial.print("Output Voltage = ");
  Serial.print(voltage);
  Serial.print("\tBattery Percentage = ");
  Serial.println(bat_percentage);

  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(voltage);
    postStr += "&field2=";
    postStr += String(bat_percentage);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    Serial.println("Data sent to ThingSpeak");
  }
  client.stop();
  delay(15000);   
}

float getAverageVoltage(int numSamples) {
  long sum = 0;
  for (int i = 0; i < numSamples; i++) {
    sum += analogRead(analogInPin);
    delay(10);
  }
  float avgSensorValue = sum / numSamples;
  return (((avgSensorValue * 3.3) / 1024) * 6 - base_calibration);
}

float calibratedVoltage(float voltage) {
  if (voltage >= 4.0 && voltage <= 4.2) {
    return voltage * 0.98;
  } else if (voltage > 4.2) {
    return voltage * 0.95;
  }
  return voltage;
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

