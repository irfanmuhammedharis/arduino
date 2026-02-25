#include <esp_now.h>
#include <WiFi.h>

// Replace with the master's MAC address
uint8_t masterMAC[] = {0xA0, 0xB7, 0x65, 0x23, 0x8E, 0x4C};

typedef struct {
  char deviceName[15];
  float latitude;
  float longitude;
} GPSDataPacket;

GPSDataPacket gpsData;

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Data sent successfully" : "Data send failed");
}

bool isPeerPaired() {
  esp_now_peer_info_t peerInfo;
  return esp_now_get_peer(masterMAC, &peerInfo) == ESP_OK;
}

bool addPeerIfNeeded() {
  if (isPeerPaired()) {
    Serial.println("Peer is already paired.");
    return true;
  } else {
    Serial.println("Peer is not paired. Attempting to pair...");
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, masterMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
      Serial.println("Peer paired successfully.");
      return true;
    } else {
      Serial.println("Failed to pair with peer.");
      return false;
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  Serial.print("This device's MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set the device name
  strcpy(gpsData.deviceName, "ambulance1"); // Change to "ambulance2" for the second ambulance
}

void loop() {
  if (addPeerIfNeeded()) {
    // Update GPS data
    gpsData.latitude = 12.971598; // Replace with actual GPS latitude
    gpsData.longitude = 77.594566; // Replace with actual GPS longitude

    // Send the GPS data
    if (esp_now_send(masterMAC, (uint8_t*)&gpsData, sizeof(gpsData)) == ESP_OK) {
      Serial.println("GPS data sent successfully!");
    } else {
      Serial.println("Failed to send GPS data.");
    }
  } else {
    Serial.println("Unable to send data. Peer not paired.");
  }

  delay(60000); // Send data every 1 minute
}
