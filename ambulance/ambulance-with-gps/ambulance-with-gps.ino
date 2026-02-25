#include <esp_now.h>
#include <WiFi.h>
#include <TinyGPSPlus.h>

// Replace with the master's MAC address
uint8_t masterMAC[] = {0x24, 0x6F, 0x28, 0xXX, 0xXX, 0xXX};

// GPS module pins
#define RXD2 16 // GPS module TX connected to ESP32 RXD2
#define TXD2 17 // GPS module RX connected to ESP32 TXD2

// TinyGPSPlus instance
TinyGPSPlus gps;

// Serial for GPS module
HardwareSerial SerialGPS(2); // Use hardware serial port 2

// Structure to send data
typedef struct {
  char deviceName[15];
  float latitude;
  float longitude;
} GPSDataPacket;

GPSDataPacket gpsData;

// Callback for when data is sent
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Data sent successfully" : "Data send failed");
}

void setup() {
  Serial.begin(115200);
  SerialGPS.begin(9600, SERIAL_8N1, RXD2, TXD2); // Initialize GPS module

  // WiFi in station mode
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Add master as a peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, masterMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Set device name
  strcpy(gpsData.deviceName, "ambulance1"); // Change to "ambulance2" for the second ambulance
}

void loop() {
  // Read data from the GPS module
  while (SerialGPS.available() > 0) {
    char c = SerialGPS.read();
    gps.encode(c); // Feed GPS data to the TinyGPSPlus parser

    // Check if GPS has a valid fix
    if (gps.location.isUpdated()) {
      gpsData.latitude = gps.location.lat();   // Get latitude
      gpsData.longitude = gps.location.lng(); // Get longitude

      // Send GPS data to the master
      esp_now_send(masterMAC, (uint8_t *)&gpsData, sizeof(gpsData));
      Serial.println("GPS data sent:");
      Serial.print("Latitude: ");
      Serial.println(gpsData.latitude, 6);
      Serial.print("Longitude: ");
      Serial.println(gpsData.longitude, 6);

      delay(60000); // Send data every 1 minute
    }
  }
}
