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
#include <SoftwareSerial.h>
#define in1 = 10
#define in2 = 9
#define in3 = 8
#define in4 = 7
#include <NewPing.h>
#include <Servo.h>

#define TRIGGER_PIN  12 
#define ECHO_PIN     11  
#define MAX_DISTANCE 400 
SoftwareSerial bluetoothSerial(A5, A4);
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 
Servo servo1;
Servo servo2;
Servo servo3;
float tempval1;
float tempval2;
int finalval;
int pos = 45;
int servoAngle1 =0;
void setup() {  
  bluetoothSerial.begin(9600);
  bluetoothSerial.println("AT+NAME NIMS");
  Serial.begin(9600);
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(7, OUTPUT);
  servo1.attach(6);
  servo2.attach(5);
  servo3.attach(3);
}

void loop() {
// delay(20);                     
//  Serial.print("Ping: ");
//  int iterations = 10;
//  tempval1=((sonar.ping_median(iterations) / 2) * 0.0343);
//  if(tempval1-tempval2>60 || tempval1-tempval2<-60)
//  {
//    tempval2=(tempval1*0.02 )+ (tempval2*0.98);
//    }
//  else
//  {
//  tempval2=(tempval1*0.4 )+ (tempval2*0.6);
//  }
//  finalval=tempval2;  
//  Serial.print(finalval);
//  if(finalval>10 && finalval<=50){
//   digitalWrite(10, LOW);
//      digitalWrite(9, LOW);
//      digitalWrite(8, LOW);
//      digitalWrite(7, LOW); 
//      Serial.println("s");
//  }
//  Serial.println("cm");
  
  if (Serial.available()) {
    char command = Serial.read();
    executeCommand(command);
    
  }
    if (bluetoothSerial.available()) {
     char command = bluetoothSerial.read();
        executeCommand(command); 
  }
  
  // Continue with other tasks in your main loop
}

void executeCommand(char command) {
  switch (command) {
    case FORWARD:
     digitalWrite(10, HIGH);
      digitalWrite(9, LOW);
      digitalWrite(8, HIGH);
      digitalWrite(7, LOW);
      Serial.println("f"); 
     
      // Perform action for moving forward
      break;
    case BACKWARD:
      digitalWrite(10, LOW);
      digitalWrite(9, HIGH);
      digitalWrite(8, LOW);
      digitalWrite(7, HIGH);
      Serial.println("b");

      break;
    case LEFT:
      digitalWrite(10, HIGH);
      digitalWrite(9, LOW);
      digitalWrite(8, LOW);
      digitalWrite(7, HIGH);
      Serial.println("L");

      break;
    case RIGHT:
       digitalWrite(10, LOW);
      digitalWrite(9, HIGH);
      digitalWrite(8, HIGH);
      digitalWrite(7, LOW);
      Serial.println("r");

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
    case PAUSE:
       digitalWrite(10, LOW);
      digitalWrite(9, LOW);
      digitalWrite(8, LOW);
      digitalWrite(7, LOW);
      Serial.println("s");// Perform action for pausing a process or operation
      break;
    default:
      digitalWrite(10, LOW);
      digitalWrite(9, LOW);
      digitalWrite(8, LOW);
      digitalWrite(7, LOW);// Invalid command received
      break;
  }
}
