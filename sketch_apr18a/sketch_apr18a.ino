#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Adafruit_MLX90614.h>
#include <Servo.h>

// Replace with your network credentials
const char* ssid = "cradle";
const char* password = "12345679";

// Initialize Telegram BOT
#define BOTtoken "6029917122:AAGU5zovLjeceu46_qFrwXUEqg8DCDetHmQ"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "1311773887"

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

Servo myservo;

int soundsensor=A0;
int watersensor=D5;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();


void setup()
{
  Serial.begin(115200);
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org

  pinMode(soundsensor,INPUT);
  pinMode(watersensor,INPUT);

  myservo.attach(2);
 
  while (!Serial);
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor. Check wiring.");
    while (1);
  };

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  bot.sendMessage(CHAT_ID, "Bot started up", "");
   Serial.print("ok ");
  
}

void loop()
{
  Serial.print("Object = ");
  Serial.print(mlx.readObjectTempC());
  Serial.println("*C");
  Serial.println();
 
  int sound=analogRead(soundsensor);
  Serial.print("Sound=");
  Serial.println(sound);

  int water=digitalRead(watersensor);
  Serial.print("Water=");
  Serial.println(water);
 
   if(mlx.readObjectTempC()>36)
   {
      bot.sendMessage(CHAT_ID, "HIGH TEMPERATURE!!", "");
   }

   if(sound>100)
   {
      int pos;

      for (pos = 0; pos <= 180; pos += 1)
      {
        // in steps of 1 degree
        myservo.write(pos);              
        delay(15);                    
      }
      for (pos = 180; pos >= 0; pos -= 1)
      {
        myservo.write(pos);              
        delay(15);                        
      }

     for (pos = 0; pos <= 180; pos += 1)
      {
        // in steps of 1 degree
        myservo.write(pos);              
        delay(15);                    
      }
      for (pos = 180; pos >= 0; pos -= 1)
      {
        myservo.write(pos);              
        delay(15);                        
      }

     for (pos = 0; pos <= 1800; pos += 1)
      {
        // in steps of 1 degree
        myservo.write(pos);              
        delay(15);                    
      }
      for (pos = 180; pos >= 0; pos -= 1)
      {
        myservo.write(pos);              
        delay(15);                        
      }

      
      bot.sendMessage(CHAT_ID, "BABY CRYING!!", "");
      delay(1000);
       
                       
      }

   

   if(water==1)
   {
      bot.sendMessage(CHAT_ID, "BABY IS WET!!", "");
   }
}
