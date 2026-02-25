#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ThingSpeak.h>

// DHT11 setup
#define DHTPIN D4       // DHT11 data pin connected to D4
#define DHTTYPE DHT11   // DHT11 sensor type
DHT dht(DHTPIN, DHTTYPE);

// MQ-135 setup
#define MQ135_PIN A0    // Analog pin connected to MQ-135 sensor output

// Wi-Fi credentials
const char* ssid = "Hexcodeplus";
const char* password = "123456789";

// ThingSpeak API information
unsigned long channelID = 2733371;     // Replace with your ThingSpeak Channel ID
const char* writeAPIKey = "Z6LMIYOOGIG1A74Y"; // Replace with your Write API Key

WiFiClient client;

void setup() {
  Serial.begin(115200);

  // Initialize DHT11
  dht.begin();

  // Initialize Wi-Fi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  // Read temperature and humidity from DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // Read air quality from MQ-135
  int mq135Value = analogRead(MQ135_PIN);

  // Check if any reading failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  // Print values to serial monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, MQ-135 Value: ");
  Serial.println(mq135Value);

  // Send values to ThingSpeak
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, mq135Value);

  int x = ThingSpeak.writeFields(channelID, writeAPIKey);
  
  if (x == 200) {
    Serial.println("Data sent to ThingSpeak successfully");
  } else {
    Serial.print("Problem sending data. HTTP error code: ");
    Serial.println(x);
  }

  // Delay between updates
  delay(20000); // Update every 20 seconds (recommended by ThingSpeak)
}
