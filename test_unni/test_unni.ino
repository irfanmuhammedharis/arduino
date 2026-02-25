// Arduino Rx --> BT Tx
#include<SoftwareSerial.h>
#include <Servo.h>
SoftwareSerial bt(6, 7); /* (Rx,Tx) */

Servo myservo1;
Servo myservo2;
Servo myservo3;
int servoPin1 = 9; // lft
int servoPin2 = 10; //head
int servoPin3 = 11; //
int pos1 = 0;
int pos2 = 0;
int pos3 = 0;


void setup()
{

    Serial.begin(9600);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  bt.begin(9600);



}
void loop()
{
  if (bt.available()>0)
  {
    char x = bt.read();
    Serial.println(x);


    switch (x)
    {
      case '1': 

   
   digitalWrite(2, HIGH);
  digitalWrite(3, LOW);
  digitalWrite(4, HIGH);
  digitalWrite(5, LOW);
  delay(1000);
  Serial.println("forward");
   
   break;
   case '2': 

   
   digitalWrite(2, LOW);
  digitalWrite(3, HIGH);
  digitalWrite(4, LOW);
  digitalWrite(5, HIGH);
  delay(1000);
  Serial.println("STOP");
   
   break;
    }

  }
}
