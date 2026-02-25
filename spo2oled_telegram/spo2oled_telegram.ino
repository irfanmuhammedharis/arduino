/*
  ESP32 MAX30102 Heart Rate Monitor - Fancy Alert Buzzer
  Author: devamika
  Login: irfanmuhammedharis
  Date: 2025-10-01 08:45:53 UTC
  
  BUZZER LOGIC (REVERSED): 
  - LOW (0V/GND) when NORMAL (ideal condition - silent)
  - HIGH (3.3V) when ALERT (abnormal - with fancy tone)
  - Plays fancy tone pattern on state change
  
  Hardware Connections:
  
  MAX30102:
    VIN  -> ESP32 3.3V, GND  -> ESP32 GND
    SDA  -> ESP32 GPIO 23, SCL  -> ESP32 GPIO 19
  
  OLED Display (128x64):
    VCC  -> ESP32 3.3V, GND  -> ESP32 GND
    SDA  -> ESP32 GPIO 16, SCL  -> ESP32 GPIO 17
  
  Buzzer (Fancy Tone on Alert):
    ESP32 GPIO 2 -> Buzzer circuit
    LOW = Normal/Ideal (silent)
    HIGH = Alert with fancy tone
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

// ========== Configuration ==========
#define WIFI_SSID "project"
#define WIFI_PASSWORD "123456789"
#define BOT_TOKEN "8248161627:AAEcRq2yh_h_O-HpvT9_olksXDnvY0k1y1A"
#define CHAT_ID "8441817891"
#define USER_NAME "devamika"
#define USER_LOGIN "irfanmuhammedharis"

// ========== Pins ==========
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C
#define MAX30102_SDA 23
#define MAX30102_SCL 19
#define OLED_SDA 16
#define OLED_SCL 17
#define BUZZER_PIN 2

// ========== Thresholds ==========
#define CRITICAL_LOW 40
#define NORMAL_MIN 60
#define NORMAL_MAX 100
#define CRITICAL_HIGH 150
#define RATE_SIZE 8
#define FINGER_THRESHOLD 50000

// Components
TwoWire I2C_OLED = TwoWire(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED, OLED_RESET);
MAX30105 particleSensor;
WiFiClientSecure client;

// Variables
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;
long irValue = 0;
bool fingerDetected = false;
bool wasFingerDetected = false;
unsigned long lastDisplayUpdate = 0;
int stableReadings = 0;

// Alert
enum AlertLevel { ALERT_NONE, ALERT_LOW, ALERT_HIGH, ALERT_CRITICAL_LOW, ALERT_CRITICAL_HIGH };
AlertLevel currentAlert = ALERT_NONE;
AlertLevel previousAlert = ALERT_NONE;
AlertLevel lastSentAlert = ALERT_NONE;

// Buzzer state
bool currentBuzzerState = LOW;   // LOW = normal (silent)
bool previousBuzzerState = LOW;

// Telegram
bool wifiConnected = false;
bool readingSent = false;
int lastAlertedBPM = 0;

// Acknowledgement
bool showAcknowledgement = false;
String acknowledgementMsg = "";
unsigned long ackStartTime = 0;
#define ACK_DISPLAY_TIME 2000

// Time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;
struct tm timeinfo;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.println("║  Heart Rate Monitor - Fancy Alert Buzzer  ║");
  Serial.println("║  User: devamika                            ║");
  Serial.println("║  Login: irfanmuhammedharis                 ║");
  Serial.println("║  Date: 2025-10-01 08:45:53 UTC             ║");
  Serial.println("╠════════════════════════════════════════════╣");
  Serial.println("║  BUZZER LOGIC (REVERSED):                  ║");
  Serial.println("║  - LOW when NORMAL (silent/ideal)          ║");
  Serial.println("║  - HIGH when ALERT (fancy tone)            ║");
  Serial.println("║  - Plays melody on state change            ║");
  Serial.println("╚════════════════════════════════════════════╝\n");
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Start silent (normal state)
  currentBuzzerState = LOW;
  previousBuzzerState = LOW;
  Serial.println("✓ Buzzer initialized: LOW (normal/silent)");

  // Initialize OLED
  I2C_OLED.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED failed!");
    while (1);
  }
  
  display.setTextColor(WHITE);
  showStartup();

  // Connect WiFi
  connectWiFi();

  if (wifiConnected) {
    // Sync time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Syncing time with NTP server...");
    delay(2000);
    
    String msg = "🏥 *Heart Rate Monitor Started*\n\n";
    msg += "✅ System: Online\n";
    msg += "📅 Date: 2025-10-01\n";
    msg += "🕐 Time: 08:45:53 UTC\n";
    msg += "👤 User: *devamika*\n";
    msg += "🔑 Login: " + String(USER_LOGIN) + "\n\n";
    msg += "⚙️ *Thresholds:*\n";
    msg += "  🔴 Critical Low: <40 BPM\n";
    msg += "  🟢 Normal: 60-100 BPM\n";
    msg += "  🔴 Critical High: >150 BPM\n\n";
    msg += "🔊 Buzzer: Fancy Alert Mode\n";
    msg += "  • LOW = Normal (silent)\n";
    msg += "  • HIGH = Alert (fancy tone)\n\n";
    msg += "👆 Ready for monitoring...";
    
    sendTelegramMessage(msg, "SYSTEM START");
  }

  // Initialize sensor
  Wire.begin(MAX30102_SDA, MAX30102_SCL);
  
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    showError();
    sendTelegramMessage("❌ *Sensor Error!*\nMAX30102 not found!\nUser: devamika\nLogin: " + String(USER_LOGIN), "ERROR");
    while (1);
  }
  
  particleSensor.setup(0x1F, 8, 2, 400, 411, 4096);
  particleSensor.setPulseAmplitudeRed(0x1F);
  
  for (int i = 0; i < RATE_SIZE; i++) rates[i] = 0;
  
  playStartupMelody();
  Serial.println("\n✓ System Ready!\n");
}

void loop() {
  // WiFi check
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
  } else {
    wifiConnected = true;
  }
  
  // Read sensor
  irValue = particleSensor.getIR();
  fingerDetected = (irValue > FINGER_THRESHOLD);
  
  if (fingerDetected && !wasFingerDetected) {
    readingSent = false;
    stableReadings = 0;
    lastAlertedBPM = 0;
    Serial.println("\n>>> Finger placed on sensor <<<");
  }
  
  if (!fingerDetected && wasFingerDetected) {
    Serial.println(">>> Finger removed <<<\n");
  }
  
  if (fingerDetected) {
    if (checkForBeat(irValue) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);
      
      if (beatsPerMinute >= 40 && beatsPerMinute <= 180 && delta > 333 && delta < 1500) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;
        
        beatAvg = 0;
        int count = 0;
        for (byte x = 0; x < RATE_SIZE; x++) {
          if (rates[x] > 0) {
            beatAvg += rates[x];
            count++;
          }
        }
        if (count > 0) {
          beatAvg /= count;
          stableReadings++;
          
          if (stableReadings == 6 && !readingSent) {
            sendHeartRateReading();
            readingSent = true;
          }
        }
      }
    }
  } else {
    beatsPerMinute = 0;
    beatAvg = 0;
    stableReadings = 0;
    previousAlert = currentAlert;
    currentAlert = ALERT_NONE;
    updateBuzzerState();
  }
  
  wasFingerDetected = fingerDetected;
  
  // Store previous alert state
  previousAlert = currentAlert;
  
  // Update alert status
  updateAlertStatus();
  
  // Update buzzer if alert state changed
  if (currentAlert != previousAlert) {
    updateBuzzerState();
    logAlertChange();
  }
  
  // Send Telegram alerts
  if (currentAlert != ALERT_NONE && stableReadings >= 6) {
    if (abs(beatAvg - lastAlertedBPM) >= 5 || currentAlert != lastSentAlert) {
      sendAlertMessage();
      lastSentAlert = currentAlert;
      lastAlertedBPM = beatAvg;
    }
  }
  
  if (currentAlert == ALERT_NONE && lastSentAlert != ALERT_NONE) {
    sendRecoveryMessage();
    lastSentAlert = ALERT_NONE;
  }
  
  // Check acknowledgement timeout
  if (showAcknowledgement && (millis() - ackStartTime > ACK_DISPLAY_TIME)) {
    showAcknowledgement = false;
  }
  
  // Update display
  if (millis() - lastDisplayUpdate >= 250) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }
  
  delay(20);
}

// ========== FANCY BUZZER FUNCTIONS ==========

void updateBuzzerState() {
  // REVERSED LOGIC:
  // LOW = Normal/Ideal (silent)
  // HIGH = Alert (with fancy tone)
  
  bool newState;
  
  if (currentAlert == ALERT_NONE) {
    newState = LOW;   // Normal = silent (LOW)
  } else {
    newState = HIGH;  // Alert = active (HIGH)
  }
  
  // Only change if state is different
  if (newState != currentBuzzerState) {
    previousBuzzerState = currentBuzzerState;
    currentBuzzerState = newState;
    
    // Play fancy tone based on transition
    if (currentBuzzerState == HIGH) {
      // Alert activated - play warning melody
      playAlertMelody();
    } else {
      // Recovered to normal - play recovery melody
      playRecoveryMelody();
    }
    
    // Set final state
    digitalWrite(BUZZER_PIN, currentBuzzerState);
    
    // Log the change
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║       BUZZER STATE CHANGED                 ║");
    Serial.println("╠════════════════════════════════════════════╣");
    Serial.printf("║  Previous: %-32s ║\n", previousBuzzerState == LOW ? "LOW (Normal)" : "HIGH (Alert)");
    Serial.printf("║  Current:  %-32s ║\n", currentBuzzerState == LOW ? "LOW (Normal)" : "HIGH (Alert)");
    Serial.printf("║  BPM:      %-32d ║\n", beatAvg);
    Serial.printf("║  Time:     %-32s ║\n", getCurrentDateTime().c_str());
    Serial.println("╚════════════════════════════════════════════╝\n");
  }
}

void playAlertMelody() {
  // Fancy alert melody - Ascending warning tones
  Serial.println("🔊 Playing ALERT melody...");
  
  int melody[] = {523, 659, 784, 988};  // C5, E5, G5, B5
  int duration = 100;
  
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], duration);
    delay(duration + 20);
    noTone(BUZZER_PIN);
    delay(30);
  }
  
  // Final urgent beep
  tone(BUZZER_PIN, 1047, 200);  // C6 - high pitch
  delay(220);
  noTone(BUZZER_PIN);
}

void playRecoveryMelody() {
  // Fancy recovery melody - Descending calming tones
  Serial.println("🔊 Playing RECOVERY melody...");
  
  int melody[] = {988, 784, 659, 523};  // B5, G5, E5, C5
  int duration = 80;
  
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], duration);
    delay(duration + 10);
    noTone(BUZZER_PIN);
    delay(20);
  }
}

void playStartupMelody() {
  // Startup melody - Happy ascending tune
  Serial.println("🔊 Playing STARTUP melody...");
  
  int melody[] = {523, 587, 659, 698, 784};  // C, D, E, F, G
  int duration = 100;
  
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, melody[i], duration);
    delay(duration + 20);
    noTone(BUZZER_PIN);
    delay(10);
  }
  
  // Final chord
  tone(BUZZER_PIN, 1047, 200);  // High C
  delay(220);
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);  // Return to silent state
}

void logAlertChange() {
  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.println("║       ALERT STATUS CHANGED                 ║");
  Serial.println("╠════════════════════════════════════════════╣");
  Serial.printf("║  Previous: %-32s ║\n", getAlertName(previousAlert).c_str());
  Serial.printf("║  Current:  %-32s ║\n", getAlertName(currentAlert).c_str());
  Serial.printf("║  BPM:      %-32d ║\n", beatAvg);
  Serial.printf("║  Buzzer:   %-32s ║\n", currentBuzzerState == LOW ? "LOW (Silent)" : "HIGH (Alert)");
  Serial.printf("║  Time:     %-32s ║\n", getCurrentDateTime().c_str());
  Serial.println("╚════════════════════════════════════════════╝\n");
}

String getAlertName(AlertLevel alert) {
  switch (alert) {
    case ALERT_NONE: return "NORMAL";
    case ALERT_LOW: return "LOW";
    case ALERT_HIGH: return "HIGH";
    case ALERT_CRITICAL_LOW: return "CRITICAL LOW";
    case ALERT_CRITICAL_HIGH: return "CRITICAL HIGH";
    default: return "UNKNOWN";
  }
}

// ========== DISPLAY FUNCTIONS ==========

void showStartup() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15, 10);
  display.print(F("HEART"));
  display.setCursor(10, 30);
  display.print(F("MONITOR"));
  display.setTextSize(1);
  display.setCursor(25, 52);
  display.print(F("by devamika"));
  display.display();
  delay(2000);
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 20);
  display.print(F("Connecting"));
  display.setCursor(30, 35);
  display.print(F("WiFi..."));
  display.display();
  
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 30) {
    delay(500);
    attempt++;
  }
  
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  
  if (wifiConnected) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(25, 20);
    display.print(F("Connected!"));
    display.setCursor(10, 35);
    display.print(F("IP: "));
    display.print(WiFi.localIP());
    display.display();
    delay(1500);
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  if (showAcknowledgement) {
    showAcknowledgementScreen();
    display.display();
    return;
  }
  
  if (!fingerDetected) {
    // Waiting screen
    display.setTextSize(1);
    display.setCursor(10, 5);
    display.print(F("PLACE FINGER ON"));
    display.setCursor(30, 20);
    display.print(F("SENSOR"));
    
    // Blinking heart
    if ((millis() / 500) % 2 == 0) {
      display.fillCircle(64, 40, 8, WHITE);
      display.fillTriangle(56, 40, 72, 40, 64, 52, WHITE);
    }
    
    // Status bar
    display.setCursor(0, 56);
    if (wifiConnected) {
      display.print(F("WiFi:OK"));
    } else {
      display.print(F("WiFi:--"));
    }
    
    display.setCursor(60, 56);
    display.print(F("Bz:"));
    display.print(currentBuzzerState == LOW ? "LO" : "HI");
    
  } else {
    // Monitoring screen
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("HEART RATE"));
    
    // WiFi indicator
    if (wifiConnected) {
      display.fillCircle(122, 3, 2, WHITE);
    }
    
    // Main BPM
    display.setTextSize(4);
    if (beatAvg > 0 && stableReadings >= 3) {
      int xPos = (beatAvg < 100) ? 30 : 10;
      display.setCursor(xPos, 15);
      display.print(beatAvg);
    } else {
      display.setTextSize(3);
      display.setCursor(35, 20);
      display.print(F("--"));
    }
    
    // BPM label
    display.setTextSize(1);
    display.setCursor(100, 35);
    display.print(F("BPM"));
    
    // Divider
    display.drawLine(0, 48, 127, 48, WHITE);
    
    // Status
    display.setTextSize(1);
    display.setCursor(0, 52);
    
    switch (currentAlert) {
      case ALERT_CRITICAL_LOW:
      case ALERT_CRITICAL_HIGH:
        if ((millis() / 300) % 2 == 0) {
          display.fillRect(0, 50, 100, 14, WHITE);
          display.setTextColor(BLACK);
          display.setCursor(10, 52);
          display.print(F("CRITICAL!"));
          display.setTextColor(WHITE);
        } else {
          display.print(F("CRITICAL!"));
        }
        break;
        
      case ALERT_LOW:
        display.print(F("Status: LOW"));
        break;
        
      case ALERT_HIGH:
        display.print(F("Status: HIGH"));
        break;
        
      case ALERT_NONE:
        if (stableReadings >= 3) {
          display.print(F("Status: NORMAL"));
        } else {
          display.print(F("Reading"));
          for (int i = 0; i < (millis() / 400) % 4; i++) {
            display.print(F("."));
          }
        }
        break;
    }
    
    // Buzzer state indicator
    display.setCursor(0, 56);
    display.print(F("Buzzer:"));
    display.print(currentBuzzerState == LOW ? "LO" : "HI");
    
    // User
    display.setCursor(90, 56);
    display.print(F("@deva"));
  }
  
  display.display();
}

void showAcknowledgementScreen() {
  display.clearDisplay();
  display.drawLine(0, 0, 127, 0, WHITE);
  display.drawLine(0, 1, 127, 1, WHITE);
  
  // Checkmark
  display.fillCircle(64, 20, 12, WHITE);
  display.setTextColor(BLACK);
  display.setTextSize(2);
  display.setCursor(58, 14);
  display.print(F("V"));
  display.setTextColor(WHITE);
  
  display.setTextSize(1);
  display.setCursor(20, 38);
  display.print(F("TELEGRAM SENT"));
  
  display.setTextSize(1);
  display.setCursor(5, 50);
  display.print(acknowledgementMsg);
  
  display.drawLine(0, 62, 127, 62, WHITE);
  display.drawLine(0, 63, 127, 63, WHITE);
}

void showError() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 15);
  display.print(F("SENSOR ERROR!"));
  display.setCursor(5, 30);
  display.print(F("MAX30102 not"));
  display.setCursor(25, 40);
  display.print(F("found!"));
  display.display();
}

void showTelegramAcknowledgement(String type) {
  showAcknowledgement = true;
  acknowledgementMsg = type;
  ackStartTime = millis();
  
  Serial.println("╔════════════════════════════════════════════╗");
  Serial.println("║        TELEGRAM MESSAGE SENT               ║");
  Serial.println("╠════════════════════════════════════════════╣");
  Serial.printf("║  Type: %-36s ║\n", type.c_str());
  Serial.printf("║  Time: %-36s ║\n", getCurrentDateTime().c_str());
  Serial.printf("║  User: %-36s ║\n", USER_NAME);
  Serial.printf("║  Login: %-33s ║\n", USER_LOGIN);
  Serial.println("╚════════════════════════════════════════════╝\n");
}

// ========== TELEGRAM FUNCTIONS ==========

void sendTelegramMessage(String message, String type) {
  if (!wifiConnected) {
    Serial.println("✗ Cannot send - WiFi not connected");
    return;
  }
  
  HTTPClient http;
  client.setInsecure();
  
  String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage";
  url += "?chat_id=" + String(CHAT_ID);
  url += "&text=" + urlEncode(message);
  url += "&parse_mode=Markdown";
  
  http.begin(client, url);
  int httpCode = http.GET();
  http.end();
  
  if (httpCode > 0) {
    Serial.println("✓ Telegram message sent successfully");
    showTelegramAcknowledgement(type);
  } else {
    Serial.printf("✗ Telegram failed. HTTP code: %d\n", httpCode);
  }
}

String urlEncode(String str) {
  String encoded = "";
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (c == ' ') encoded += "%20";
    else if (c == '\n') encoded += "%0A";
    else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') encoded += c;
    else {
      char buf[4];
      sprintf(buf, "%%%02X", c);
      encoded += buf;
    }
  }
  return encoded;
}

void sendHeartRateReading() {
  String msg = "📊 *Heart Rate Reading*\n\n";
  msg += "❤️ BPM: *" + String(beatAvg) + "*\n";
  msg += "📅 2025-10-01\n";
  msg += "🕐 " + getCurrentDateTime() + " UTC\n";
  msg += "👤 User: *devamika*\n";
  msg += "🔑 Login: " + String(USER_LOGIN) + "\n\n";
  
  if (beatAvg < CRITICAL_LOW) {
    msg += "🚨 Status: *CRITICAL LOW*\n🔊 Buzzer: HIGH (Alert)";
  } else if (beatAvg < NORMAL_MIN) {
    msg += "⚠️ Status: *LOW*\n🔊 Buzzer: HIGH (Alert)";
  } else if (beatAvg <= NORMAL_MAX) {
    msg += "✅ Status: *NORMAL*\n🔊 Buzzer: LOW (Silent)";
  } else if (beatAvg <= CRITICAL_HIGH) {
    msg += "⚠️ Status: *HIGH*\n🔊 Buzzer: HIGH (Alert)";
  } else {
    msg += "🚨 Status: *CRITICAL HIGH*\n🔊 Buzzer: HIGH (Alert)";
  }
  
  msg += "\n\n📌 Normal: 60-100 BPM";
  
  sendTelegramMessage(msg, "BPM: " + String(beatAvg));
  Serial.printf("=== Reading Sent: %d BPM ===\n", beatAvg);
}

void sendAlertMessage() {
  String msg = "🚨 *HEART RATE ALERT!* 🚨\n\n";
  msg += "⚠️ Abnormal reading detected!\n\n";
  msg += "❤️ BPM: *" + String(beatAvg) + "*\n";
  msg += "📅 2025-10-01\n";
  msg += "🕐 " + getCurrentDateTime() + " UTC\n";
  msg += "👤 User: *devamika*\n";
  msg += "🔑 Login: " + String(USER_LOGIN) + "\n\n";
  
  String alertType = "";
  
  switch (currentAlert) {
    case ALERT_CRITICAL_LOW:
      msg += "🚨🚨🚨 *CRITICAL LOW*\n";
      msg += "Below 40 BPM\n";
      msg += "🔊 Buzzer: HIGH (Alert Tone)\n";
      msg += "⚠️ Seek medical help NOW!";
      alertType = "CRITICAL LOW";
      break;
    case ALERT_LOW:
      msg += "⚠️ *LOW HEART RATE*\n";
      msg += "Below 60 BPM\n";
      msg += "🔊 Buzzer: HIGH (Alert Tone)\n";
      msg += "💡 Monitor closely";
      alertType = "LOW ALERT";
      break;
    case ALERT_HIGH:
      msg += "⚠️ *HIGH HEART RATE*\n";
      msg += "Above 100 BPM\n";
      msg += "🔊 Buzzer: HIGH (Alert Tone)\n";
      msg += "💡 Rest and monitor";
      alertType = "HIGH ALERT";
      break;
    case ALERT_CRITICAL_HIGH:
      msg += "🚨🚨🚨 *CRITICAL HIGH*\n";
      msg += "Above 150 BPM\n";
      msg += "🔊 Buzzer: HIGH (Alert Tone)\n";
      msg += "⚠️ Seek medical help NOW!";
      alertType = "CRITICAL HIGH";
      break;
  }
  
  sendTelegramMessage(msg, alertType);
  Serial.printf("!!! ALERT SENT: %s - %d BPM !!!\n", alertType.c_str(), beatAvg);
}

void sendRecoveryMessage() {
  String msg = "✅ *Heart Rate Recovered*\n\n";
  msg += "Heart rate returned to normal\n\n";
  msg += "❤️ BPM: *" + String(beatAvg) + "*\n";
  msg += "📅 2025-10-01\n";
  msg += "🕐 " + getCurrentDateTime() + " UTC\n";
  msg += "👤 User: *devamika*\n";
  msg += "🔑 Login: " + String(USER_LOGIN) + "\n\n";
  msg += "✅ Status: *NORMAL*\n";
  msg += "🔊 Buzzer: LOW (Silent)\n";
  msg += "🎵 Recovery melody played\n";
  msg += "📌 Normal range: 60-100 BPM";
  
  sendTelegramMessage(msg, "RECOVERY");
  Serial.println("=== Recovery notification sent ===\n");
}

String getCurrentDateTime() {
  if (!getLocalTime(&timeinfo)) {
    return "08:45:53";
  }
  
  char buffer[10];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  return String(buffer);
}

void updateAlertStatus() {
  if (beatAvg > 0 && stableReadings >= 6) {
    if (beatAvg < CRITICAL_LOW) currentAlert = ALERT_CRITICAL_LOW;
    else if (beatAvg < NORMAL_MIN) currentAlert = ALERT_LOW;
    else if (beatAvg <= NORMAL_MAX) currentAlert = ALERT_NONE;
    else if (beatAvg <= CRITICAL_HIGH) currentAlert = ALERT_HIGH;
    else currentAlert = ALERT_CRITICAL_HIGH;
  } else if (!fingerDetected) {
    currentAlert = ALERT_NONE;
  }
}