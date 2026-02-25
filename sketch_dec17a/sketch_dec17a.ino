
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// 1. Define Network and Bot Credentials
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"
#define BOT_TOKEN "XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

const unsigned long BOT_MTBS = 1000; // Mean time between scan messages (1 second)

// 2. Instantiate Secure Client and Bot Object
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

unsigned long bot_lasttime;

void handleNewMessages(int numNewMessages) {
  // Logic for handling messages, e.g., echoing the message text
  for (int i = 0; i < numNewMessages; i++) {
    bot.sendMessage(bot.messages[i].chat_id, bot.messages[i].text, ""); 
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  // --- Step 1: Establish WiFi Connection ---
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status()!= WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  // --- Step 2: Mandatory NTP Synchronization and Blocking Wait ---
  // Guarantees temporal validity required for X.509 certificate check.
  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  
  time_t now = time(nullptr);
  // Block execution until time is established beyond the 1970 Epoch start
  while (now < 24 * 3600) { 
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.print("\nTime established (Epoch: ");
  Serial.print(now);
  Serial.println(")");
  
  // --- Step 3: Load Root CA Trust Anchor ---
  // Explicitly set the trust anchor for api.telegram.org.
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); 

  // --- Step 4: Bot Initialization (Configuration Complete) ---
  // The bot is now ready to securely connect.
}

void loop() {
  if (millis() - bot_lasttime > BOT_MTBS) {
    // Check for new messages every 1 second
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }
}