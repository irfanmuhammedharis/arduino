
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

const char* ssid = "project";
const char* password = "12345678";

#define BOTtoken "6155780563:AAExWPl1RBX_T4GJlmo0uL_vTgKiqMCp8uU"  // your Bot Token (Get from Botfather)


#define CHAT_ID "1010741328"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

void setup() {
Serial.begin(9600);
Serial.print("Connecting Wifi: ");
Serial.println(ssid);

WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password);
client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org

while (WiFi.status() != WL_CONNECTED) {
  Serial.print(".");
  delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  bot.sendMessage(CHAT_ID, "Bot started up", "");
}

void loop() {

int sensor1=analogRead(34);
int sensor2=analogRead(35);
int sensor3=analogRead(32);
int sensor4=analogRead(33);
int sensors1=map(sensor1,0,4065,0,1023);
int sensors2=map(sensor2,0,4065,0,1023);
int sensors3=map(sensor3,0,4065,0,1023);
int sensors4=map(sensor4,0,4065,0,1023);

Serial.println(sensors1);
Serial.println(sensors2);
Serial.println(sensors3);
Serial.println(sensors4);
delay(1000);
if(sensors1<200&&sensors2>500)
{
  bot.sendMessage(CHAT_ID, "pressure difference in right shoulder and left shoulder", "");
  Serial.println("shoulder");
}
else if(sensors2<200&&sensors1>500)
{
  bot.sendMessage(CHAT_ID, "pressure difference in right shoulder and left shoulder", "");
  Serial.println("shoulder");
}
else if(sensors3<200&&sensors4>500)
{
 bot.sendMessage(CHAT_ID, "pressure difference in left hip and right hip", "");
 Serial.println("hip");
}
else if(sensors4<200&&sensors3>500)
{
 bot.sendMessage(CHAT_ID, "pressure difference in left hip and right hip", "");
 Serial.println("hip");
} 
}
