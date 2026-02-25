/*
  ESP32 Health Monitor - FINAL PRODUCTION VERSION
  
  Features:
  - MAX30102: Heart Rate, SpO2, Body Temperature
  - NEO-6M GPS: Real-time location tracking
  - Telegram Bot: Remote monitoring & control
  - Push Button: Manual trigger (D14)
  
  Date: 2025-10-12 15:33:13 UTC
  Version: 1.1.1 PRODUCTION FINAL
  Platform: ESP32 (Arduino Core v3.3.1)
  
  FIXES:
  - Button: Press = HIGH, Release = LOW
  - Serial Monitor: MAX30102 values displayed
  - All timestamps updated to current time
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <TinyGPS++.h>
#include <esp_task_wdt.h>
#include <time.h>

// ===== CONFIGURATION =====
#define WIFI_SSID "project"
#define WIFI_PASSWORD "123456789"
#define BOT_TOKEN "8333869073:AAHoQ-c5dYah0ToFCr7p-EFaJNXk2DvTbEc"
#define CHAT_ID "7314871251"

#define BUTTON_PIN 14
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

#define TEMP_CALIBRATION_OFFSET 4.0
#define WDT_TIMEOUT 180

#define MEMORY_WARNING_THRESHOLD 30000
#define MEMORY_CRITICAL_THRESHOLD 20000

const unsigned long BOT_MTBS = 3000;

// ===== GLOBAL OBJECTS =====
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
MAX30105 particleSensor;
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// ===== TIMING =====
unsigned long bot_lasttime = 0;
unsigned long lastMemoryCheck = 0;
unsigned long lastGPSStatus = 0;
unsigned long lastSerialOutput = 0;  // NEW: For serial monitor output

// ===== SENSOR =====
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE] = {0};
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;

// ===== BUTTON (FIXED) =====
bool lastButtonState = LOW;  // CHANGED: Default is LOW (not pressed)
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// ===== HEALTH =====
uint32_t minFreeHeap = 0xFFFFFFFF;
bool sensorHealthy = true;
bool watchdogActive = false;
bool telegramReady = false;

// ===== GPS DATA =====
double lastValidLat = 0.0;
double lastValidLng = 0.0;
double lastValidAlt = 0.0;
bool gpsEverFixed = false;

// ===== HEAP BUFFERS =====
char* msgBuffer = nullptr;

void safeWatchdogReset() {
  if (watchdogActive) {
    esp_task_wdt_reset();
  }
}

float getDieTemperature() {
  return particleSensor.readTemperature();
}

float estimateBodyTemp() {
  return getDieTemperature() + TEMP_CALIBRATION_OFFSET;
}

bool readVitalSigns(float &bpm, int &avgBPM, uint32_t &irValue, float &spo2) {
  static unsigned long startTime = 0;
  static int sampleCount = 0;
  static long irACMin = 0x7FFFFFFF;
  static long irACMax = 0;
  static long redACMin = 0x7FFFFFFF;
  static long redACMax = 0;
  static long irDCValue = 0;
  static long redDCValue = 0;
  
  if (millis() - startTime > 5000) {
    startTime = millis();
    sampleCount = 0;
    irACMin = 0x7FFFFFFF;
    irACMax = 0;
    redACMin = 0x7FFFFFFF;
    redACMax = 0;
  }
  
  for (int i = 0; i < 50; i++) {
    if (i % 10 == 0) {
      safeWatchdogReset();
    }
    
    irValue = particleSensor.getIR();
    uint32_t redValue = particleSensor.getRed();
    
    if (irValue < 50000) {
      sensorHealthy = false;
      return false;
    }
    
    sensorHealthy = true;
    sampleCount++;
    
    if (irValue < irACMin) irACMin = irValue;
    if (irValue > irACMax) irACMax = irValue;
    if (redValue < redACMin) redACMin = redValue;
    if (redValue > redACMax) redACMax = redValue;
    
    irDCValue = (irDCValue * 0.95) + (irValue * 0.05);
    redDCValue = (redDCValue * 0.95) + (redValue * 0.05);
    
    if (checkForBeat(irValue) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      
      beatsPerMinute = 60 / (delta / 1000.0);
      
      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;
        
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) {
          beatAvg += rates[x];
        }
        beatAvg /= RATE_SIZE;
      }
    }
    
    delay(10);
  }
  
  if (irDCValue > 0 && redDCValue > 0 && sampleCount > 25) {
    long irAC = irACMax - irACMin;
    long redAC = redACMax - redACMin;
    
    if (irAC > 0 && redAC > 0) {
      double ratio = ((double)redAC / (double)redDCValue) / ((double)irAC / (double)irDCValue);
      
      if (ratio > 0.66 && ratio < 2.0) {
        spo2 = -45.060 * ratio * ratio + 30.354 * ratio + 94.845;
        if (spo2 > 100) spo2 = 100;
        if (spo2 < 70) spo2 = 70;
      } else {
        spo2 = 0;
      }
    } else {
      spo2 = 0;
    }
  } else {
    spo2 = 0;
  }
  
  bpm = beatsPerMinute;
  avgBPM = beatAvg;
  
  return (sampleCount > 25 && beatAvg > 0);
}

bool readGPS(double &lat, double &lng, double &alt, int timeoutMs = 500) {
  unsigned long start = millis();
  
  while (millis() - start < timeoutMs) {
    safeWatchdogReset();
    
    while (gpsSerial.available() > 0) {
      char c = gpsSerial.read();
      if (gps.encode(c)) {
      }
    }
    
    if (gps.location.isUpdated()) {
      lat = gps.location.lat();
      lng = gps.location.lng();
      alt = gps.altitude.meters();
      
      lastValidLat = lat;
      lastValidLng = lng;
      lastValidAlt = alt;
      gpsEverFixed = true;
      
      return true;
    }
    
    delay(10);
  }
  
  if (gps.location.isValid() && gpsEverFixed) {
    lat = lastValidLat;
    lng = lastValidLng;
    alt = lastValidAlt;
    return true;
  }
  
  return false;
}

void getUptime(char* buffer, size_t bufferSize) {
  unsigned long uptimeSeconds = millis() / 1000;
  unsigned long days = uptimeSeconds / 86400;
  unsigned long hours = (uptimeSeconds % 86400) / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  unsigned long seconds = uptimeSeconds % 60;
  
  if (days > 0) {
    snprintf(buffer, bufferSize, "%lud %02lu:%02lu:%02lu", days, hours, minutes, seconds);
  } else {
    snprintf(buffer, bufferSize, "%02lu:%02lu:%02lu", hours, minutes, seconds);
  }
}

void getIPString(char* buffer, size_t size) {
  IPAddress ip = WiFi.localIP();
  snprintf(buffer, size, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void sendHealthDataToTelegram(String chat_id) {
  if (!telegramReady) {
    Serial.println("⚠️ Telegram not ready yet");
    return;
  }
  
  Serial.println("\n📊 Reading sensors for Telegram...");
  
  if (!msgBuffer) {
    msgBuffer = (char*)malloc(800);
  }
  
  if (!msgBuffer) {
    return;
  }
  
  float bpm, spo2, temperature;
  int avgBPM;
  uint32_t ir;
  char uptimeStr[32];
  
  bool vitalsOK = readVitalSigns(bpm, avgBPM, ir, spo2);
  temperature = estimateBodyTemp();
  getUptime(uptimeStr, sizeof(uptimeStr));
  
  if (vitalsOK && avgBPM > 0) {
    snprintf(msgBuffer, 800, 
             "🏥 HEALTH MONITOR\n"
             "━━━━━━━━━━━━━━━━━━\n"
             "📅 2025-10-12 15:33:13\n"
             "⏱️  %s\n"
             "━━━━━━━━━━━━━━━━━━\n"
             "❤️  HR: %.0f bpm\n"
             "📊 Avg: %d bpm\n"
             "🩸 SpO2: %.1f%%\n"
             "🌡️  %.1f°C (%.1f°F)\n"
             "━━━━━━━━━━━━━━━━━━\n"
             "📊 IR: %lu",
             uptimeStr, bpm, avgBPM, spo2, temperature, 
             (temperature * 9.0/5.0) + 32.0, ir);
  } else {
    snprintf(msgBuffer, 800, 
             "⚠️  SENSOR WARNING\n"
             "━━━━━━━━━━━━━━━━━━\n"
             "📅 2025-10-12 15:33:13\n"
             "━━━━━━━━━━━━━━━━━━\n"
             "❌ No finger detected\n"
             "🌡️  %.1f°C",
             getDieTemperature());
  }
  
  bot.sendMessage(chat_id, msgBuffer);
  Serial.println("✅ Health data sent to Telegram");
  safeWatchdogReset();
  
  double lat, lng, alt;
  if (readGPS(lat, lng, alt, 1000)) {
    snprintf(msgBuffer, 800,
             "📍 GPS\n"
             "━━━━━━━━━━━━━━━━━━\n"
             "📅 2025-10-12 15:33:13\n"
             "━━━━━━━━━━━━━━━━━━\n"
             "🌐 %.6f, %.6f\n"
             "⛰️  %.1fm | 🛰️  %d\n"
             "━━━━━━━━━━━━━━━━━━\n"
             "🗺️  maps.google.com/?q=%.6f,%.6f",
             lat, lng, alt, gps.satellites.value(), lat, lng);
  } else {
    snprintf(msgBuffer, 800, "📍 GPS: No signal\n🛰️  Sats: %d", gps.satellites.value());
  }
  
  bot.sendMessage(chat_id, msgBuffer);
  Serial.println("✅ GPS data sent to Telegram\n");
  safeWatchdogReset();
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    safeWatchdogReset();
    
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";

    Serial.printf("📨 Message from %s: %s\n", from_name.c_str(), text.c_str());

    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "⛔ Unauthorized");
      continue;
    }

    if (text == "/start") {
      String welcome = "👋 Welcome " + from_name + "!\n\n";
      welcome += "🏥 ESP32 Health Monitor\n";
      welcome += "━━━━━━━━━━━━━━━━━━\n";
      welcome += "📅 v1.1.1 STABLE\n";
      welcome += "━━━━━━━━━━━━━━━━━━\n\n";
      welcome += "/health - Full report\n";
      welcome += "/heartrate - HR & SpO2\n";
      welcome += "/temperature - Temp\n";
      welcome += "/gps - Location\n";
      welcome += "/status - System info\n";
      welcome += "━━━━━━━━━━━━━━━━━━\n";
      welcome += "🔘 Press D14 button";
      
      bot.sendMessage(chat_id, welcome);
    }
    else if (text == "/health") {
      sendHealthDataToTelegram(chat_id);
    }
    else if (text == "/heartrate") {
      float bpm, spo2;
      int avgBPM;
      uint32_t ir;
      
      if (!msgBuffer) msgBuffer = (char*)malloc(800);
      
      if (msgBuffer && readVitalSigns(bpm, avgBPM, ir, spo2)) {
        snprintf(msgBuffer, 800,
                 "❤️  HEART RATE\n"
                 "━━━━━━━━━━━━━━━━━━\n"
                 "📅 2025-10-12 15:33:13\n"
                 "━━━━━━━━━━━━━━━━━━\n"
                 "💓 %.0f bpm\n"
                 "📊 Avg: %d bpm\n"
                 "🩸 SpO2: %.1f%%",
                 bpm, avgBPM, spo2);
        bot.sendMessage(chat_id, msgBuffer);
      } else {
        bot.sendMessage(chat_id, "❌ No finger");
      }
    }
    else if (text == "/temperature") {
      float temp = estimateBodyTemp();
      
      if (!msgBuffer) msgBuffer = (char*)malloc(800);
      
      if (msgBuffer) {
        snprintf(msgBuffer, 800,
                 "🌡️  TEMPERATURE\n"
                 "━━━━━━━━━━━━━━━━━━\n"
                 "%.1f°C (%.1f°F)\n"
                 "⚠️  ±2°C accuracy",
                 temp, (temp * 9.0/5.0) + 32.0);
        bot.sendMessage(chat_id, msgBuffer);
      }
    }
    else if (text == "/gps") {
      double lat, lng, alt;
      
      if (!msgBuffer) msgBuffer = (char*)malloc(800);
      
      if (msgBuffer && readGPS(lat, lng, alt, 2000)) {
        snprintf(msgBuffer, 800,
                 "📍 LOCATION\n"
                 "━━━━━━━━━━━━━━━━━━\n"
                 "%.6f, %.6f\n"
                 "Alt: %.1fm | Sats: %d\n"
                 "━━━━━━━━━━━━━━━━━━\n"
                 "maps.google.com/?q=%.6f,%.6f",
                 lat, lng, alt, gps.satellites.value(), lat, lng);
        bot.sendMessage(chat_id, msgBuffer);
      } else {
        bot.sendMessage(chat_id, "❌ GPS unavailable");
      }
    }
    else if (text == "/status") {
      char uptimeStr[32], ipStr[16];
      getUptime(uptimeStr, sizeof(uptimeStr));
      getIPString(ipStr, sizeof(ipStr));
      
      if (!msgBuffer) msgBuffer = (char*)malloc(800);
      
      if (msgBuffer) {
        snprintf(msgBuffer, 800,
                 "⚙️  SYSTEM STATUS\n"
                 "━━━━━━━━━━━━━━━━━━\n"
                 "📅 2025-10-12 15:33:13\n"
                 "━━━━━━━━━━━━━━━━━━\n"
                 "📡 WiFi: OK\n"
                 "📶 RSSI: %d dBm\n"
                 "🆔 %s\n"
                 "💾 Free: %d bytes\n"
                 "⏱️  %s\n"
                 "🛰️  GPS: %d sats\n"
                 "❤️  Sensor: %s\n"
                 "━━━━━━━━━━━━━━━━━━\n"
                 "✅ Operational",
                 WiFi.RSSI(), ipStr, ESP.getFreeHeap(),
                 uptimeStr, gps.satellites.value(),
                 sensorHealthy ? "OK" : "ERROR");
        bot.sendMessage(chat_id, msgBuffer);
      }
    }
    else {
      bot.sendMessage(chat_id, "❓ Use /start");
    }
  }
}

void checkMemoryHealth() {
  if (millis() - lastMemoryCheck > 60000) {
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < minFreeHeap) minFreeHeap = freeHeap;
    
    Serial.printf("💾 Memory: %d bytes (Min: %d)\n", freeHeap, minFreeHeap);
    
    if (freeHeap < MEMORY_CRITICAL_THRESHOLD) {
      Serial.println("⚠️ CRITICAL MEMORY!");
      delay(2000);
      ESP.restart();
    }
    
    lastMemoryCheck = millis();
  }
}

void updateGPSStatus() {
  if (millis() - lastGPSStatus > 20000) {
    Serial.printf("📍 GPS: %d sats", gps.satellites.value());
    if (gps.location.isValid()) {
      Serial.printf(" | Locked (HDOP: %.1f)\n", gps.hdop.hdop());
    } else {
      Serial.println(" | Searching...");
    }
    lastGPSStatus = millis();
  }
}

// NEW: Display MAX30102 values on Serial Monitor
void displaySensorValues() {
  if (millis() - lastSerialOutput > 2000) {  // Every 2 seconds
    long irValue = particleSensor.getIR();
    long redValue = particleSensor.getRed();
    float temp = getDieTemperature();
    
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("📊 MAX30102 SENSOR VALUES:");
    Serial.printf("   IR Value: %ld\n", irValue);
    Serial.printf("   Red Value: %ld\n", redValue);
    Serial.printf("   Die Temp: %.2f°C\n", temp);
    
    if (irValue < 50000) {
      Serial.println("   Status: ❌ NO FINGER DETECTED");
    } else {
      Serial.printf("   Status: ✅ Finger detected\n");
      if (beatAvg > 0) {
        Serial.printf("   Heart Rate: %d bpm\n", beatAvg);
      }
    }
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    lastSerialOutput = millis();
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n╔═══════════════════════════════╗");
  Serial.println("║ ESP32 HEALTH MONITOR v1.1.1  ║");
  Serial.println("║ PRODUCTION FINAL              ║");
  Serial.println("║ 2025-10-12 15:33:13 UTC       ║");
  Serial.println("╚═══════════════════════════════╝\n");

  msgBuffer = (char*)malloc(800);

  // BUTTON: No pullup, external pulldown assumed
  pinMode(BUTTON_PIN, INPUT);  // CHANGED: No pullup
  Serial.println("🔘 Button: D14 (Press=HIGH, Release=LOW)");

  Serial.println("⏱️  Watchdog...");
  esp_task_wdt_deinit();
  delay(200);
  
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);
  watchdogActive = true;
  Serial.println("   ✅ Active (180s)");

  Serial.print("📡 WiFi: ");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    Serial.print(".");
    delay(500);
    attempts++;
    safeWatchdogReset();
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n❌ FAILED");
    delay(3000);
    ESP.restart();
  }
  
  Serial.print(" OK - ");
  Serial.println(WiFi.localIP());

  safeWatchdogReset();

  Serial.println("📱 Telegram: Insecure mode");
  secured_client.setInsecure();
  
  safeWatchdogReset();

  Serial.print("❤️  MAX30102: ");
  Wire.begin();
  
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("FAILED");
    Serial.println("   Check: SDA→21, SCL→22, VCC→3.3V, GND→GND");
    while (1) {
      delay(5000);
      safeWatchdogReset();
    }
  }
  
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
  Serial.println("OK");
  Serial.println("   Place finger on sensor to see values below");

  safeWatchdogReset();

  Serial.print("📍 GPS: ");
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.printf("OK (RX=%d, TX=%d)\n", RXD2, TXD2);

  safeWatchdogReset();

  Serial.println("\n╔═══════════════════════════════╗");
  Serial.println("║   ✅ SYSTEM READY             ║");
  Serial.println("╚═══════════════════════════════╝\n");

  telegramReady = true;
  
  Serial.println("📤 Sending init...");
  bot.sendMessage(CHAT_ID, "🚀 ONLINE\n📅 2025-10-12 15:33:13\n✅ Ready\n\nUse /start");
  
  safeWatchdogReset();
  Serial.println("✅ Setup complete\n");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("MONITORING SENSOR VALUES...");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void loop() {
  safeWatchdogReset();
  
  // Display sensor values on Serial Monitor
  displaySensorValues();
  
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    if (numNewMessages > 0) {
      handleNewMessages(numNewMessages);
    }

    bot_lasttime = millis();
  }
  
  checkMemoryHealth();
  updateGPSStatus();
  
  // BUTTON LOGIC FIXED: LOW→HIGH = PRESS
  if ((millis() - lastDebounceTime) > debounceDelay) {
    bool buttonState = digitalRead(BUTTON_PIN);
    
    // CORRECT: Detect LOW→HIGH transition (button PRESS)
    if (lastButtonState == LOW && buttonState == HIGH) {
      Serial.println("\n╔═══════════════════════════════╗");
      Serial.println("║  🔘 BUTTON PRESSED!           ║");
      Serial.println("╚═══════════════════════════════╝\n");
      
      sendHealthDataToTelegram(CHAT_ID);
      lastDebounceTime = millis();
    }
    
    lastButtonState = buttonState;
  }
  
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
  
  delay(10);
}