#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
WiFiClient client;
SoftwareSerial mySerial(1, 3); //SIM800L Tx & Rx is connected to Arduino #3 & #2
void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA); // set WiFi mode to station
  WiFi.begin("myAP", "password"); // connect to access point with SSID and password
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  sendSms();
}

void loop() {
  if (client.connect("192.168.4.1", 80)) { // connect to server with IP address and port
    client.println("Hello from client"); // send a message to the server
    client.flush();
    while (client.connected()) {
      if (client.available()) {
        String message = client.readStringUntil('\n'); // read incoming data
        Serial.println("Received message: " + message);
      }
    }
    client.stop();
  }
  while (WiFi.status() == WL_CONNECTED) {
    delay(1000);
    Serial.println("send sms...");
    sendSms();
  }
  delay(1000);
}
void updateSerial()
{
  delay(500);
  while (Serial.available()) 
  {
    mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(mySerial.available()) 
  {
    Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port
  }
}
void sendSms (){
   Serial.println("sending sms"); 
  delay(1000);

  mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
  updateSerial();

  mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
 mySerial.println("AT+CMGS=\"+919633118080\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  mySerial.print("student cross gate"); //text content
  updateSerial();
  mySerial.write(26);
}
