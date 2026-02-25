#include <SoftwareSerial.h>

//Create software serial object to communicate with SIM800L
SoftwareSerial mySerial(3, 2); //SIM800L Tx & Rx is connected to Arduino #3 & #2
int inputPin = 4;
bool inputState = LOW;
bool hasBlinked = false; // initialize flag for blinking to false
int ledPin = 13; 
 int sign=34;
void setup()
{

  pinMode(inputPin,INPUT);
  Serial.begin(9600);
  mySerial.begin(9600);
  Serial.println("Initializing..."); 
  delay(1000);
  mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
  updateSerial();
  mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
 mySerial.println("AT+CMGS=\"+919633118080\"");
  updateSerial();
  mySerial.print("welcome "); //text content
  updateSerial();
  mySerial.write(26);
}

void loop()
{
int value = digitalRead(inputPin); // read current state of input pin
if(value==1){

  Serial.println("pin is HIGH");
 
  if(sign==34){
  sendSms();
  }
  else{
    Serial.println("loop");
    }
  sign=365;
}
  else{
    sign=34;
    Serial.println("pin is LOW");
    }
    
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
void sendSms(){
  Serial.println("Sending sms");
  mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
  updateSerial();
  mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
 mySerial.println("AT+CMGS=\"+919633118080\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  mySerial.print("student cross gate "); //text content
  Serial.println("sms sended");
  updateSerial();
  mySerial.write(26);
  delay(1000);
}
