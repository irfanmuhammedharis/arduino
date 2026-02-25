#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
WiFiServer server(80);
SoftwareSerial mySerial(3, 2);
void setup() {
  Serial.println("divice is connected");
  Serial.begin(9600);
  WiFi.mode(WIFI_AP); // set WiFi mode to access point
  WiFi.softAP("myAP", "password"); // set up access point with SSID and password
  IPAddress myIP = WiFi.softAPIP();
  server.begin();
  Serial.println("Server IP address: " + myIP.toString());
}

void loop() {
  WiFiClient client = server.available(); // listen for incoming connections
  if (client) {
    Serial.println("New client connected");
    while (client.connected()) {
      if (client.available()) {
        sendSms();
        String message = client.readStringUntil('\n'); // read incoming data
        Serial.println("Received message: " + message);
        client.println("Received message: " + message); // send a response
      }
    }
    client.stop();
    Serial.println("Client disconnected");
  }
}
void sendSms (){
   Serial.println("sending sms"); 
  delay(1000);

  mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
  

  mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
  
 mySerial.println("AT+CMGS=\"+919633118080\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  
  mySerial.print("student cross gate"); //text content
  
  mySerial.write(26);
}
