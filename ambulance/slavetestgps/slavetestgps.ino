#include <esp_now.h>
#include <WiFi.h>
#include <TinyGPSPlus.h>

// Replace with your receiver MAC Address
uint8_t broadcastAddress[] = {0xA0, 0xB7, 0x65, 0x23, 0x8E, 0x4C};

// Create a TinyGPS++ object
TinyGPSPlus gps;

// Structure to send GPS data
typedef struct struct_message {
  int id;
  double latitude;
  double longitude;
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;

// GPS Serial pins
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
HardwareSerial gpsSerial(1); // UART1 for GPS

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  // Initialize GPS Serial
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register Send Callback
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // Parse GPS data
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c); // Feed data into TinyGPS++
  }

  // Check if GPS location is valid
  if (gps.location.isUpdated()) {
    myData.id = 1; // Unique ID for the sender
    myData.latitude = gps.location.lat();
    myData.longitude = gps.location.lng();

    // Print location to Serial Monitor
    Serial.print("Latitude: ");
    Serial.print(myData.latitude, 6);
    Serial.print(", Longitude: ");
    Serial.println(myData.longitude, 6);

    // Send GPS data via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK) {
      Serial.println("Sent with success");
    } else {
      Serial.println("Error sending the data");
    }
  } else {
    Serial.println("Waiting for valid GPS data...");
  }

  delay(10000); // Delay for 2 seconds
}
