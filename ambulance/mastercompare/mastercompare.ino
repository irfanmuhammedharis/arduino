#include <esp_now.h>
#include <WiFi.h>
#include <math.h> // For calculating distance

// Structure to receive GPS data
typedef struct struct_message {
  int id;         // Device ID or message ID
  double latitude;  // Latitude value
  double longitude; // Longitude value
} struct_message;

// Instance of struct_message
struct_message myData;

// Variables to store locations of devices
struct_message device1Data;
struct_message device2Data;

// Flags to check if data from devices 1 and 2 has been received
bool device1Received = false;
bool device2Received = false;

// Function to calculate distance between two coordinates (Haversine formula)
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371e3; // Earth's radius in meters
  double phi1 = radians(lat1);
  double phi2 = radians(lat2);
  double deltaPhi = radians(lat2 - lat1);
  double deltaLambda = radians(lon2 - lon1);

  double a = sin(deltaPhi / 2) * sin(deltaPhi / 2) +
             cos(phi1) * cos(phi2) *
             sin(deltaLambda / 2) * sin(deltaLambda / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));

  return R * c; // Distance in meters
}

// Callback function for received data
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len == sizeof(myData)) {
    memcpy(&myData, incomingData, sizeof(myData));
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Device ID: ");
    Serial.println(myData.id);
    Serial.print("Latitude: ");
    Serial.println(myData.latitude, 6);
    Serial.print("Longitude: ");
    Serial.println(myData.longitude, 6);
    Serial.println();

    // Store data based on device ID
    if (myData.id == 1) {
      device1Data = myData;
      device1Received = true;
    } else if (myData.id == 2) {
      device2Data = myData;
      device2Received = true;
    } else if (myData.id == 3) {
      if (device1Received && device2Received) {
        // Calculate distances
        double distanceToDevice1 = calculateDistance(myData.latitude, myData.longitude, device1Data.latitude, device1Data.longitude);
        double distanceToDevice2 = calculateDistance(myData.latitude, myData.longitude, device2Data.latitude, device2Data.longitude);

        // Determine the nearest device
        if (distanceToDevice1 < distanceToDevice2) {
          Serial.println("Nearest device: Device 1");
        } else {
          Serial.println("Nearest device: Device 2");
        }
      } else {
        Serial.println("Waiting for data from devices 1 and 2...");
      }
    }
  } else {
    Serial.println("Data size mismatch!");
  }
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
