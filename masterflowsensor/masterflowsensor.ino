#include <esp_now.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>  // Use I2C LCD library

// Declare the I2C LCD object with address 0x27 (common address, may vary)
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Address 0x27, 16 columns, 2 rows

const int flowPin = 13; // Flow sensor pin (choose free digital pin)
volatile unsigned long pulseCount = 0;
const float pulses_per_liter = 450.0;

float main_total_liters = 0.0;
float slave1_total = 0.0;
float slave2_total = 0.0;
const int led = 2;
typedef struct {
  uint8_t slave_id;
  float total_liters;
} Data;

void IRAM_ATTR pulseISR() {
  pulseCount++;
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Data data;
  memcpy(&data, incomingData, sizeof(data));
  if (data.slave_id == 1) {
    slave1_total = data.total_liters;
    Serial.print("Received from Slave 1: ");
    Serial.print(slave1_total);
    Serial.println(" L");
  } else if (data.slave_id == 2) {
    slave2_total = data.total_liters;
    Serial.print("Received from Slave 2: ");
    Serial.print(slave2_total);
    Serial.println(" L");
  }
}

void setup() {
  pinMode(led,OUTPUT);
  digitalWrite(led,LOW);
  Serial.begin(115200);
  Serial.println("Starting Master ESP32...");

  WiFi.mode(WIFI_STA);
  Serial.println("WiFi initialized in STA mode");

  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("Master MAC Address: ");
  for (int i = 0; i < 6; i++) {
    if (i > 0) Serial.print(", ");
    Serial.print("0x");
    if (mac[i] < 0x10) Serial.print("0");
    Serial.print(mac[i], HEX);
  }
  Serial.println();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("ESP-NOW initialized");

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW receive callback registered");

  // Initialize I2C LCD
  lcd.init();          // Initialize the LCD
  lcd.backlight();     // Turn on the backlight
  Serial.println("I2C LCD initialized");

  pinMode(flowPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(flowPin), pulseISR, RISING);
  Serial.println("Flow sensor interrupt attached");
}

void loop() {
  static unsigned long lastUpdateTime = 0;
  if (millis() - lastUpdateTime >= 1000) {
    lastUpdateTime = millis();
    main_total_liters = pulseCount / pulses_per_liter;

    Serial.println("--- Flow Update ---");
    Serial.print("Main Sensor: ");
    Serial.print(main_total_liters);
    Serial.println(" L");
    Serial.print("Slave 1: ");
    Serial.print(slave1_total);
    Serial.println(" L");
    Serial.print("Slave 2: ");
    Serial.print(slave2_total);
    Serial.println(" L");
    Serial.print("Max Usage: ");
    if (slave1_total > slave2_total) {
      Serial.print("Slave 1 (");
      Serial.print(slave1_total);
      Serial.println(" L)");
    } else {
      Serial.print("Slave 2 (");
      Serial.print(slave2_total);
      Serial.println(" L)");
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Main: ");
    lcd.print(main_total_liters, 1);
    lcd.print(" L");
    if (main_total_liters>500)
    {
      digitalWrite(led, HIGH);
    }
    lcd.setCursor(0, 1);
    if (slave1_total > slave2_total) {
      lcd.print("Max: S1 ");
      lcd.print(slave1_total, 1);
      lcd.print(" L");
    } else {
      lcd.print("Max: S2 ");
      lcd.print(slave2_total, 1);
      lcd.print(" L");
    }
  }
}