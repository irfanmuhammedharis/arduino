/* 
   motor 
   in1 12
   in2 14
   in3 26
   in4 27
  servo
  servo1 2 
  servo2 21 
  servo3 22 
  servo4 23 
  servo5 15 
  
*/
#define FORWARD 'F'
#define BACKWARD 'B'
#define LEFT 'L'
#define RIGHT 'R'
#define CIRCLE 'C'
#define CROSS 'X'
#define TRIANGLE 'T'
#define SQUARE 'S'
#define START 'A'
#define PAUSE 'P'
#include <SPI.h>
#include <ESP32Servo.h>  // Include the ESP32Servo library instead of Servo
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;
const int TRIGGER_PIN = 4;
const int ECHO_PIN = 13;
//#define TRIGGER_PIN  4 
//#define ECHO_PIN     13 
//define sound speed in cm/uS
#define SOUND_SPEED 0.034

long duration;
float distanceCm; 
#define MAX_DISTANCE 400 
 
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;
Servo servo5;
int pos = 45;
int posh = 90;
int servoAngle1 =0;
#define right1 12
#define right2 14
#define left1 27
#define left2 26 
#define led 5 
   void ultrasoic()
      {
        digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  // Sets the TRIGGER_PIN on HIGH state for 10 micro seconds
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  // Reads the ECHO_PIN, returns the sound wave travel time in microseconds
  duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  
  
  
  // Prints the distance in the Serial Monitor
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  
  delay(1000); 
      }

      void action()
   {
    for (pos = 0; pos <= 90; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo1.write(pos);  
    //servo2.write(pos);
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  for (pos = 0; pos <= 90; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    //servo1.write(pos);  
    servo2.write(pos);
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  for (pos = 0; pos <= 90; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo4.write(pos);  
    servo5.write(pos);
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  for (pos = 90; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    servo4.write(pos);
    servo5.write(pos);
    delay(15);    
    Serial.println("finger servo");
   }
    for (posh = 90; posh <= 120; posh += 1) 
    { 
    servo3.write(posh);  
     delay(15);                       // waits 15 ms for the servo to reach the position
    }
     for (posh = 120; posh >= 90; posh -= 1) { // goes from 180 degrees to 0 degrees
    servo3.write(posh);
    delay(15);    
    Serial.println("head");
   }
   for (posh = 90; posh <= 120; posh += 1) 
    { 
    servo3.write(posh);  
     delay(15);                       // waits 15 ms for the servo to reach the position
    }
     for (posh = 120; posh >= 90; posh -= 1) { // goes from 180 degrees to 0 degrees
    servo3.write(posh);
    delay(15);    
    Serial.println("head");
   }
    
  for (pos = 90; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    servo1.write(pos);
    servo2.write(pos);
    delay(15);    
    Serial.println("hand servo");
   }
   }
void setup()
{
 SerialBT.begin("robo");
  Serial.begin(115200);
  pinMode(right1, OUTPUT);
  pinMode(right2, OUTPUT);
  pinMode(left1, OUTPUT);
  pinMode(left2, OUTPUT);
  servo1.attach(2);
  servo2.attach(21);
  servo3.attach(22); 
  servo4.attach(23);
  servo5.attach(15); 
  pinMode(TRIGGER_PIN, OUTPUT); // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT); // Sets the echoPin as an Input
  pinMode(led,OUTPUT);
}

void loop()
{
  digitalWrite(led,HIGH);
  delay(2000);
   digitalWrite(led,LOW);
  delay(2000);
  
//ultrasoic();
 if (Serial.available()) {
    char command = Serial.read();
    executeCommand(command);
    
  }
    if (SerialBT.available()) {
     char command = SerialBT.read();
        executeCommand(command); 
  }
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  // Sets the TRIGGER_PIN on HIGH state for 10 micro seconds
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  // Reads the ECHO_PIN, returns the sound wave travel time in microseconds
  duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  
  
  
  // Prints the distance in the Serial Monitor
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
if (distanceCm<10)
{
  action();
}

}
void executeCommand(char command) {
  switch (command) {
    case FORWARD:
     digitalWrite(right1, HIGH);
      digitalWrite(right2, LOW);
      digitalWrite(left1, HIGH);
      digitalWrite(left2, LOW);
      Serial.println("f"); 
     
      // Perform action for moving forward
      break;
    case BACKWARD:
      digitalWrite(right1, LOW);
      digitalWrite(right2, HIGH);
      digitalWrite(8, LOW);
      digitalWrite(7, HIGH);
      Serial.println("b");
 
      break;
    case LEFT:
      digitalWrite(right1, HIGH);
      digitalWrite(right2, LOW);
      digitalWrite(left1, LOW);
      digitalWrite(left2, HIGH);
      Serial.println("L");
 
      break;
    case RIGHT:
       digitalWrite(right1, LOW);
      digitalWrite(right2, HIGH);
      digitalWrite(left1, HIGH);
      digitalWrite(left2, LOW);
      Serial.println("r");
 
      break;
      case PAUSE:
       digitalWrite(right1, LOW);
      digitalWrite(right2, LOW);
      digitalWrite(left1, LOW);
      digitalWrite(left2, LOW);
      Serial.println("s");// Perform action for pausing a process or operation
      break;
    default:
      digitalWrite(right1, LOW);
      digitalWrite(right2, LOW);
      digitalWrite(left1, LOW);
      digitalWrite(left2, LOW);// Invalid command received
      break;
      case CIRCLE:
       for (pos = 0; pos <= 90; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo1.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  for (pos = 90; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    servo1.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);    
    Serial.println("left servo");// waits 15 ms for the servo to reach the position
  }// Perform action for circle
      break;
    case CROSS:
       for (pos = 45; pos <= 90; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo2.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  for (pos = 90; pos >= 45; pos -= 1) { // goes from 180 degrees to 0 degrees
    servo2.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);  
      Serial.println("right servo");// waits 15 ms for the servo to reach the position
  }
  for (pos = 45; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    servo2.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);  
      Serial.println("right servo");// waits 15 ms for the servo to reach the position
  }
   for (pos = 0; pos <= 45; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo2.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  // Perform action for immediate stop or crossing
      break;
    case TRIANGLE:
      for (pos = 0; pos <= 90; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo3.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  for (pos = 90; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    servo3.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);  
    Serial.println("head");// waits 15 ms for the servo to reach the position
  }// Perform action for toggling a state (e.g., LED on/off)
      break;
    case SQUARE:
       servoAngle1 += 15; // Increment the servo angle by 1 degree
    if (servoAngle1 > 180) {
      servoAngle1 = 180; // Limit the angle to 0-180 degrees
    }
    servo1.write(servoAngle1);
    delay(20);
    // delay(1000);// Perform action for retrieving and sending status information
      break;
    case START:
       servoAngle1 -= 15; // Increment the servo angle by 1 degree
    if (servoAngle1 < 0) {
      servoAngle1 =0; // Limit the angle to 0-180 degrees
    }
    servo1.write(servoAngle1);// Perform action for starting a process or operation
   // delay(1000);
   delay(20);
      break;
      }}
