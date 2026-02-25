#include <esp_now.h>
#include <WiFi.h>

// -------------------- CONFIGURATION --------------------

#define SLAVE_ID 2 // Unique ID for this slave device

// YF-S201 flow sensor is connected to GPIO27
const int flowPin = 13;


// Sensor spec: YF-S201 = ~450 pulses per liter
const float pulses_per_liter = 450.0;

// MAC address of the master ESP32 (replace with actual)
uint8_t masterMac[] = {0x88, 0x13, 0xBF, 0x00, 0xF8, 0x84};

// -------------------- DATA STRUCTURE --------------------

typedef struct {
  uint8_t slave_id;
  float total_liters;
} Data;

Data data;

// -------------------- FLOW SENSOR VARIABLES --------------------

volatile unsigned long pulseCount = 0;
volatile unsigned long lastPulseTime = 0; // For debounce

bool peerRegistered = false;

// -------------------- INTERRUPT ROUTINE --------------------

void IRAM_ATTR pulseISR() {
  unsigned long now = micros();
  if (now - lastPulseTime > 500) { // Debounce: ignore pulses <500µs apart
    pulseCount++;
    lastPulseTime = now;
  }
}

// -------------------- SETUP --------------------

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 Flow Sensor Slave...");

  // Setup WiFi in STA mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("Slave MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while (true);
  }
  Serial.println("ESP-NOW initialized");

  // Register peer (master)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, masterMac, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    while (true);
  }
  Serial.println("Master peer added");
  peerRegistered = true;

  // Initialize flow sensor pin and interrupt
  pinMode(flowPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(flowPin), pulseISR, RISING);
  Serial.println("Flow sensor interrupt attached to GPIO27");
}

// -------------------- MAIN LOOP --------------------

void loop() {
  static unsigned long lastSendTime = 0;

  // Send total liters every 1 second
  if (millis() - lastSendTime >= 1000) {
    lastSendTime = millis();

    if (!peerRegistered || !esp_now_is_peer_exist(masterMac)) {
      Serial.println("Master peer not found");
      peerRegistered = false;
      return;
    }

    // Convert pulses to liters (keep accumulating total)
    float totalLiters = pulseCount / pulses_per_liter;

    data.slave_id = SLAVE_ID;
    data.total_liters = totalLiters;

    Serial.printf("Sending to Master - Slave %d: %.3f L (total)\n", SLAVE_ID, totalLiters);

    esp_err_t result = esp_now_send(masterMac, (uint8_t *)&data, sizeof(data));

    if (result == ESP_OK) {
      Serial.println("Data sent successfully");
    } else {
      Serial.println("Failed to send data");
      peerRegistered = false;
    }

    // ⚠️ DO NOT RESET pulseCount — we want running total
    // pulseCount = 0; <-- removed
  }
}
