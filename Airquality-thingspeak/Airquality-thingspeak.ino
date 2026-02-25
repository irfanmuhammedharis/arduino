#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// Define ThingSpeak API details
const char* ssid = "GKL";         // Replace with your network credentials
const char* password = "333555888"; 
const char* apiKey = "M106MDX5YREKXYYN";   // Replace with your ThingSpeak API Key
const char* server = "http://api.thingspeak.com/update";

// Sensor Pin Definitions
#define DHTPIN 14  // GPIO pin for DHT11
#define DHTTYPE DHT11
#define MQ7_PIN 32  // Analog pin for MQ7
#define MQ135_PIN 33  // Analog pin for MQ135

// Create a DHT object
DHT dht(DHTPIN, DHTTYPE);

// Function to connect to WiFi
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected!");
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  connectToWiFi();
}

void loop() {
  // Read DHT11 data
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Read MQ7 (Carbon Monoxide) data
  int mq7_value = analogRead(MQ7_PIN);  // Analog value from 0 to 4095

  // Read MQ135 (Air Quality) data
  int mq135_value = analogRead(MQ135_PIN);  // Analog value from 0 to 4095

  // Check if any reads failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Print values to the serial monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, MQ7 CO Level: ");
  Serial.print(mq7_value);
  Serial.print(", MQ135 Air Quality: ");
  Serial.println(mq135_value);

  // Send data to ThingSpeak
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey + "&field1=" + String(temperature)
                 + "&field2=" + String(humidity) + "&field3=" + String(mq7_value) 
                 + "&field4=" + String(mq135_value);
                 
    http.begin(url);
    int httpCode = http.GET();  // Send GET request

    // Check the response
    if (httpCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpCode);
    } else {
      Serial.print("Error on sending data: ");
      Serial.println(httpCode);
    }

    http.end();  // Free resources
  }

  // Wait 20 seconds before sending next data (to comply with ThingSpeak free account limits)
  delay(20000);
}
