/*
  ESP32 MAX30102 Heart Rate Monitor with Attractive OLED Display
  Author: devamika
  Date: 2025-10-01
  User: irfanmuhammedharis
  
  Features:
  - Modern attractive OLED display with animations
  - Real-time heart rate monitoring
  - Telegram notifications
  - 5V Buzzer alerts
  - Smooth transitions and visual effects
  
  Hardware Connections:
  
  MAX30102:
    VIN  -> ESP32 3.3V, GND  -> ESP32 GND
    SDA  -> ESP32 GPIO 23, SCL  -> ESP32 GPIO 19
  
  OLED Display:
    VCC  -> ESP32 3.3V, GND  -> ESP32 GND
    SDA  -> ESP32 GPIO 16, SCL  -> ESP32 GPIO 17
  
  5V Buzzer (with NPN transistor):
    ESP32 GPIO 2 -> 1kΩ -> Transistor Base
    5V -> Buzzer (+), Buzzer (-) -> Transistor Collector
    Transistor Emitter -> GND
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

// ========== Display Configuration ==========
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

// ========== I2C Configuration ==========
#define MAX30102_SDA 23
#define MAX30102_SCL 19
#define OLED_SDA 16
#define OLED_SCL 17
#define BUZZER_PIN 2

// ========== Heart Rate Thresholds ==========
#define CRITICAL_LOW 40
#define NORMAL_MIN 60
#define NORMAL_MAX 100
#define CRITICAL_HIGH 150
#define RATE_SIZE 8
#define FINGER_THRESHOLD 50000
#define MIN_BPM 40
#define MAX_BPM 180

// Initialize components
TwoWire I2C_OLED = TwoWire(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED, OLED_RESET);
MAX30105 particleSensor;
WiFiClientSecure client;

// Heart rate variables
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

// Alert system
enum AlertLevel { ALERT_NONE, ALERT_LOW, ALERT_HIGH, ALERT_CRITICAL_LOW, ALERT_CRITICAL_HIGH };
AlertLevel currentAlert = ALERT_NONE;
AlertLevel lastSentAlert = ALERT_NONE;
unsigned long lastBuzzerToggle = 0;

// Telegram variables
bool wifiConnected = false;
bool readingSent = false;
int lastAlertedBPM = 0;

// Animation variables
int pulseRadius = 0;
int waveOffset = 0;
int heartbeatAnim = 0;

// Time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

// ========== Enhanced Icons ==========
// Large animated heart (24x24)
static const unsigned char heart_large[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc7, 0x00, 0x07, 0xef, 0x80,
  0x0f, 0xff, 0xc0, 0x1f, 0xff, 0xe0, 0x3f, 0xff, 0xf0, 0x7f, 0xff, 0xf8,
  0x7f, 0xff, 0xf8, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xfc,
  0xff, 0xff, 0xfc, 0x7f, 0xff, 0xf8, 0x7f, 0xff, 0xf8, 0x3f, 0xff, 0xf0,
  0x1f, 0xff, 0xe0, 0x0f, 0xff, 0xc0, 0x07, 0xff, 0x80, 0x03, 0xff, 0x00,
  0x01, 0xfe, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x78, 0x00, 0x00, 0x30, 0x00
};

// WiFi icon
static const unsigned char wifi_icon[] PROGMEM = {
  0x00, 0x00, 0x1f, 0xf8, 0x3f, 0xfc, 0x70, 0x0e,
  0x0f, 0xf0, 0x1f, 0xf8, 0x18, 0x18, 0x03, 0xc0,
  0x07, 0xe0, 0x04, 0x20, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};

// ECG wave pattern
static const unsigned char ecg_wave[] PROGMEM = {
  0x80, 0x80, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04,
  0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08,
  0x10, 0x20, 0x40, 0x80, 0x80, 0x80, 0x80, 0x80
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.println("║  ESP32 Heart Rate Monitor - Premium UI    ║");
  Serial.println("║  User: devamika | Login: irfanmuhammedharis║");
  Serial.println("║  Date: 2025-10-01 07:41:32 UTC             ║");
  Serial.println("╚════════════════════════════════════════════╝\n");

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize OLED
  I2C_OLED.begin(OLED_SDA, OLED_SCL);
  I2C_OLED.setClock(400000);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED failed!");
    while (1);
  }
  
  // Animated startup sequence
  showAnimatedStartup();

  // Connect WiFi with progress animation
  connectToWiFiWithAnimation();

  if (wifiConnected) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(2000);
    
    String startupMsg = "🏥 *Heart Rate Monitor Started*\n\n";
    startupMsg += "✅ System: Online\n";
    startupMsg += "📅 Date: 2025-10-01\n";
    startupMsg += "🕐 Time: 07:41:32 UTC\n";
    startupMsg += "👤 User: *devamika*\n";
    startupMsg += "🔑 Login: irfanmuhammedharis\n";
    startupMsg += "📊 Status: Active\n";
    startupMsg += "🔔 Alerts: Enabled\n\n";
    startupMsg += "⚙️ *Thresholds:*\n";
    startupMsg += "  🔴 Critical Low: <40 BPM\n";
    startupMsg += "  🟢 Normal: 60-100 BPM\n";
    startupMsg += "  🔴 Critical High: >150 BPM\n\n";
    startupMsg += "👆 Place finger on sensor...";
    
    sendTelegramMessage(startupMsg);
  }

  // Initialize MAX30102
  Wire.begin(MAX30102_SDA, MAX30102_SCL);
  
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    showError();
    sendTelegramMessage("❌ *SENSOR ERROR*\nMAX30102 not found!\nUser: devamika");
    while (1);
  }
  
  particleSensor.setup(0x1F, 8, 2, 400, 411, 4096);
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);
  
  for (int i = 0; i < RATE_SIZE; i++) rates[i] = 0;
  
  testBuzzer();
  
  Serial.println("✓ System Ready! Place finger on sensor...\n");
}

void loop() {
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      wifiConnected = false;
      connectToWiFiWithAnimation();
    }
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
  }
  
  if (fingerDetected) {
    if (checkForBeat(irValue) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);
      
      if (beatsPerMinute >= MIN_BPM && beatsPerMinute <= MAX_BPM && delta > 333 && delta < 1500) {
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
    currentAlert = ALERT_NONE;
    stopBuzzer();
  }
  
  wasFingerDetected = fingerDetected;
  
  updateAlertStatus();
  handleBuzzerPattern();
  
  if (currentAlert != ALERT_NONE && stableReadings >= 6) {
    if (abs(beatAvg - lastAlertedBPM) >= 5 || currentAlert != lastSentAlert) {
      sendAlertMessage();
      lastSentAlert = currentAlert;
      lastAlertedBPM = beatAvg;
    }
  }
  
  if (currentAlert == ALERT_NONE) lastSentAlert = ALERT_NONE;
  
  // Update display with animations
  if (millis() - lastDisplayUpdate >= 50) {  // 20 FPS for smooth animation
    lastDisplayUpdate = millis();
    updateAttractiveDisplay();
  }
  
  delay(20);
}

// ========== ATTRACTIVE DISPLAY FUNCTIONS ==========

void showAnimatedStartup() {
  // Fade in effect
  for (int brightness = 0; brightness < 3; brightness++) {
    display.clearDisplay();
    
    // Large heart animation
    for (int size = 0; size <= 24; size += 4) {
      display.clearDisplay();
      int x = 64 - size/2;
      int y = 20 - size/2;
      display.fillCircle(x, y, size/2, WHITE);
      display.fillCircle(x + size, y, size/2, WHITE);
      display.fillTriangle(x - size/2, y, x + size + size/2, y, x + size/2, y + size, WHITE);
      display.display();
      delay(30);
    }
    
    display.clearDisplay();
    display.drawBitmap(52, 10, heart_large, 24, 24, WHITE);
    
    // Title with typewriter effect
    String title = "HEART MONITOR";
    for (int i = 0; i <= title.length(); i++) {
      display.fillRect(0, 40, 128, 10, BLACK);
      display.setTextSize(1);
      display.setCursor(20, 40);
      display.print(title.substring(0, i));
      display.display();
      delay(50);
    }
    
    // Subtitle
    display.setTextSize(1);
    display.setCursor(15, 52);
    display.print("Premium Edition");
    display.display();
    delay(800);
  }
}

void connectToWiFiWithAnimation() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 30) {
    display.clearDisplay();
    
    // WiFi icon
    display.drawBitmap(56, 5, wifi_icon, 16, 16, WHITE);
    
    // Title
    display.setTextSize(1);
    display.setCursor(25, 25);
    display.print("CONNECTING");
    
    // Animated progress bar
    int progress = (attempt * 100) / 30;
    display.drawRect(14, 40, 100, 8, WHITE);
    display.fillRect(16, 42, progress * 96 / 100, 4, WHITE);
    
    // Percentage
    display.setCursor(50, 52);
    display.print(progress);
    display.print("%");
    
    display.display();
    delay(500);
    attempt++;
  }
  
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  
  if (wifiConnected) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(30, 25);
    display.print("CONNECTED!");
    display.drawRect(14, 40, 100, 8, WHITE);
    display.fillRect(16, 42, 96, 4, WHITE);
    display.display();
    delay(1000);
  }
}

void updateAttractiveDisplay() {
  display.clearDisplay();
  
  // Update animation counters
  waveOffset = (waveOffset + 2) % 128;
  
  if (!fingerDetected) {
    showWaitingScreen();
  } else {
    showMonitoringScreen();
  }
  
  display.display();
}

void showWaitingScreen() {
  // Animated pulsing heart
  pulseRadius = (pulseRadius + 1) % 20;
  int heartSize = 12 + (pulseRadius / 2);
  
  // Draw pulsing circles around center
  for (int i = 0; i < 3; i++) {
    int radius = heartSize + (i * 6) + (pulseRadius % 6);
    if (radius < 30) {
      display.drawCircle(64, 20, radius, WHITE);
    }
  }
  
  // Center heart icon
  display.drawBitmap(52, 8, heart_large, 24, 24, WHITE);
  
  // Instructional text with modern styling
  display.setTextSize(1);
  
  // Box around instructions
  display.drawRoundRect(8, 35, 112, 26, 4, WHITE);
  
  display.setCursor(15, 40);
  display.print("PLACE FINGER ON");
  display.setCursor(30, 50);
  display.print("SENSOR PAD");
  
  // WiFi status indicator (top right corner)
  if (wifiConnected) {
    display.fillCircle(120, 4, 2, WHITE);
    display.drawCircle(120, 4, 4, WHITE);
  }
}

void showMonitoringScreen() {
  // Top bar with WiFi and status
  display.drawLine(0, 10, 128, 10, WHITE);
  
  if (wifiConnected) {
    display.drawBitmap(110, 0, wifi_icon, 16, 16, WHITE);
  }
  
  // Animated heartbeat icon
  heartbeatAnim = (millis() - lastBeat) < 200 ? 1 : 0;
  
  if (heartbeatAnim) {
    // Filled heart on beat
    display.fillCircle(8, 5, 4, WHITE);
    display.fillTriangle(4, 5, 12, 5, 8, 10, WHITE);
  } else {
    // Regular heart
    display.drawCircle(8, 5, 4, WHITE);
    display.drawTriangle(4, 5, 12, 5, 8, 10, WHITE);
  }
  
  // Main BPM Display with modern styling
  display.setTextSize(1);
  display.setCursor(20, 2);
  display.print("HEART RATE");
  
  // Large BPM number with shadow effect
  display.setTextSize(3);
  int xPos = (beatAvg < 100) ? 45 : 35;
  
  if (beatAvg > 0 && stableReadings >= 3) {
    // Shadow
    display.setCursor(xPos + 1, 17);
    display.setTextColor(WHITE);
    display.print(beatAvg);
    
    // Main number
    display.setCursor(xPos, 16);
    display.print(beatAvg);
  } else {
    display.setTextSize(2);
    display.setCursor(48, 20);
    display.print("--");
  }
  
  // BPM label
  display.setTextSize(1);
  display.setCursor(100, 24);
  display.print("BPM");
  
  // ECG-style waveform
  drawECGWave();
  
  // Status bar at bottom
  display.drawLine(0, 52, 128, 52, WHITE);
  
  // Status text with icons
  display.setTextSize(1);
  display.setCursor(2, 55);
  
  switch (currentAlert) {
    case ALERT_CRITICAL_LOW:
    case ALERT_CRITICAL_HIGH:
      // Blinking warning
      if ((millis() / 200) % 2 == 0) {
        display.fillRect(0, 53, 128, 11, WHITE);
        display.setTextColor(BLACK);
        display.setCursor(25, 55);
        display.print("!!! CRITICAL !!!");
        display.setTextColor(WHITE);
      }
      break;
      
    case ALERT_LOW:
      display.print("LOW  ");
      drawMiniGraph(30, 55, beatAvg, NORMAL_MIN);
      break;
      
    case ALERT_HIGH:
      display.print("HIGH ");
      drawMiniGraph(30, 55, beatAvg, NORMAL_MAX);
      break;
      
    case ALERT_NONE:
      if (stableReadings >= 3) {
        display.write(3); // Heart symbol
        display.print(" NORMAL ");
        display.write(3);
        
        // Progress indicator
        int progress = constrain(stableReadings * 10, 0, 60);
        display.fillRect(68, 57, progress, 5, WHITE);
      } else {
        display.print("READING");
        // Loading animation
        for (int i = 0; i < (millis() / 200) % 4; i++) {
          display.print(".");
        }
      }
      break;
  }
  
  // User indicator (bottom right)
  display.setTextSize(1);
  display.setCursor(90, 55);
  display.print("@deva");
}

void drawECGWave() {
  // Draw animated ECG-like waveform
  int baseline = 44;
  
  for (int x = 0; x < 128; x++) {
    int amplitude = 0;
    
    if (fingerDetected && beatAvg > 0) {
      // Create wave pattern
      int pos = (x + waveOffset) % 128;
      
      // R-peak (tall spike)
      if (pos >= 60 && pos <= 65) {
        amplitude = -12 + (pos - 60) * 4;
        if (pos == 63) amplitude = -20; // Peak
      }
      // P-wave (small bump before R)
      else if (pos >= 50 && pos <= 55) {
        amplitude = -2;
      }
      // T-wave (bump after R)
      else if (pos >= 70 && pos <= 80) {
        amplitude = -3;
      }
    }
    
    display.drawPixel(x, baseline + amplitude, WHITE);
  }
  
  // Baseline
  display.drawLine(0, baseline, 128, baseline, WHITE);
}

void drawMiniGraph(int x, int y, int current, int reference) {
  // Mini bar graph showing deviation
  int barHeight = abs(current - reference) / 5;
  barHeight = constrain(barHeight, 0, 8);
  
  if (current < reference) {
    display.fillRect(x, y + 8 - barHeight, 20, barHeight, WHITE);
  } else {
    display.fillRect(x, y, 20, barHeight, WHITE);
  }
}

void showError() {
  display.clearDisplay();
  
  // Error icon (X in circle)
  display.drawCircle(64, 20, 15, WHITE);
  display.drawLine(54, 10, 74, 30, WHITE);
  display.drawLine(74, 10, 54, 30, WHITE);
  
  display.setTextSize(1);
  display.setCursor(20, 40);
  display.print("SENSOR ERROR!");
  display.setCursor(10, 52);
  display.print("Check MAX30102");
  display.display();
}

// ========== TELEGRAM & UTILITY FUNCTIONS ==========

void sendTelegramMessage(String message) {
  if (!wifiConnected) return;
  
  HTTPClient http;
  client.setInsecure();
  
  String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage";
  url += "?chat_id=" + String(CHAT_ID);
  url += "&text=" + urlEncode(message);
  url += "&parse_mode=Markdown";
  
  http.begin(client, url);
  http.GET();
  http.end();
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
  msg += "❤️ Heart Rate: *" + String(beatAvg) + " BPM*\n";
  msg += "📅 2025-10-01 | 🕐 07:41:32 UTC\n";
  msg += "👤 User: *devamika*\n";
  msg += "🔑 Login: irfanmuhammedharis\n\n";
  msg += getStatusText();
  msg += "\n\n📌 Normal: 60-100 BPM";
  sendTelegramMessage(msg);
}

void sendAlertMessage() {
  String msg = "🚨 *ALERT!* 🚨\n\n";
  msg += "❤️ BPM: *" + String(beatAvg) + "*\n";
  msg += "📅 2025-10-01 | 🕐 07:41:32 UTC\n";
  msg += "👤 User: *devamika*\n\n";
  msg += getAlertText();
  sendTelegramMessage(msg);
}

String getStatusText() {
  if (beatAvg < CRITICAL_LOW) return "🚨 *CRITICAL LOW*";
  if (beatAvg < NORMAL_MIN) return "⚠️ *LOW*";
  if (beatAvg <= NORMAL_MAX) return "✅ *NORMAL*";
  if (beatAvg <= CRITICAL_HIGH) return "⚠️ *HIGH*";
  return "🚨 *CRITICAL HIGH*";
}

String getAlertText() {
  switch (currentAlert) {
    case ALERT_CRITICAL_LOW: return "🚨 CRITICAL LOW (<40 BPM)\n⚠️ Seek medical help NOW!";
    case ALERT_LOW: return "⚠️ LOW (<60 BPM)\n💡 Monitor closely";
    case ALERT_HIGH: return "⚠️ HIGH (>100 BPM)\n💡 Rest and monitor";
    case ALERT_CRITICAL_HIGH: return "🚨 CRITICAL HIGH (>150 BPM)\n⚠️ Seek medical help NOW!";
    default: return "";
  }
}

void updateAlertStatus() {
  if (beatAvg > 0 && stableReadings >= 6) {
    if (beatAvg < CRITICAL_LOW) currentAlert = ALERT_CRITICAL_LOW;
    else if (beatAvg < NORMAL_MIN) currentAlert = ALERT_LOW;
    else if (beatAvg <= NORMAL_MAX) currentAlert = ALERT_NONE;
    else if (beatAvg <= CRITICAL_HIGH) currentAlert = ALERT_HIGH;
    else currentAlert = ALERT_CRITICAL_HIGH;
  } else {
    currentAlert = ALERT_NONE;
  }
}

void handleBuzzerPattern() {
  switch (currentAlert) {
    case ALERT_CRITICAL_LOW:
    case ALERT_CRITICAL_HIGH:
      if (millis() - lastBuzzerToggle >= 150) {
        lastBuzzerToggle = millis();
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
      }
      break;
    case ALERT_LOW:
    case ALERT_HIGH:
      if (millis() - lastBuzzerToggle >= 1000) {
        lastBuzzerToggle = millis();
        digitalWrite(BUZZER_PIN, HIGH);
        delay(300);
        digitalWrite(BUZZER_PIN, LOW);
      }
      break;
    default:
      digitalWrite(BUZZER_PIN, LOW);
      break;
  }
}

void stopBuzzer() {
  digitalWrite(BUZZER_PIN, LOW);
}

void testBuzzer() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

String getCurrentTime() {
  return "07:41:32";
}