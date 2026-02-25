#include <esp_now.h>
#include <WiFi.h>

// Structure to receive GPS data
typedef struct struct_message {
  int id;         // Device ID or message ID
  double latitude;  // Latitude value
  double longitude; // Longitude value
} struct_message;

// Instance of struct_message
struct_message myData;

// Callback function for received data
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len == sizeof(myData)) {
    memcpy(&myData, incomingData, sizeof(myData));
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Device ID: ");
    Serial.println(myData.id);
    Serial.print("Latitude: ");
    Serial.println(myData.latitude, 6); // Print with 6 decimal places
    Serial.print("Longitude: ");
    Serial.println(myData.longitude, 6); // Print with 6 decimal places
    Serial.println();

    // Forward GPS data to ESP8266
    forwardToESP8266(myData);
  } else {
    Serial.println("Data size mismatch!");
  }
}

void forwardToESP8266(struct_message data) {
  Serial2.print(data.id);
  Serial2.print(",");
  Serial2.print(data.latitude, 6); // 6 decimal places for latitude
  Serial2.print(",");
  Serial2.println(data.longitude, 6); // 6 decimal places for longitude
}

void setup() {
  Serial.begin(115200);

  // Initialize Serial2 for ESP8266
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  // Set device to Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback for data reception
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // Loop intentionally left empty
}
