#include <SoftwareSerial.h>

//Create software serial object to communicate with SIM800L
SoftwareSerial mySerial(3,2); //SIM800L Tx & Rx is connected to Arduino #3 & #2
#define button1 4 //Button pin, on the other pin it's wired with GND

bool button_State; //Button state
int token = 0;


void setup()
{
  //Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  pinMode(button1, INPUT_PULLUP);
  //Begin serial communication with Arduino and SIM800L
  mySerial.begin(9600);

  Serial.println("Initializing..."); 
  delay(1000);

  mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
  updateSerial();
  mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
 mySerial.println("AT+CMGS=\"+919633118080\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  mySerial.print("Hi welcome to students tracking"); //text content
  updateSerial();
  mySerial.write(26);
}

void loop()
{
   button_State = digitalRead(button1);   //We are constantly reading the button State
 
  if (button_State == HIGH) {            //And if it's pressed
    Serial.println("Button pressed");   //Shows this message on the serial monitor
    delay(200);  //Small delay to avoid detecting the button press many times
    if (token==0)
    {
    sendSms();  //And this function is called
    token=1;
    }
  
}
token=0;
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
