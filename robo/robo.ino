
#define in1 = 10
#define in2 = 9
#define in3 = 8
#define in4 = 7
#include <NewPing.h>
#include <Servo.h>

#define TRIGGER_PIN  12 
#define ECHO_PIN     11  
#define MAX_DISTANCE 400 
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 
Servo servo1;
Servo servo2;
Servo servo3;
float tempval1;
float tempval2;
int finalval;
int pos = 0;
void setup() {
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
//   delay(20);                     
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
  
  if (Serial.available() > 0) {
    char inputvalue = char(Serial.read());
    if (inputvalue == 'F') {
      digitalWrite(10, HIGH);
      digitalWrite(9, LOW);
      digitalWrite(8, HIGH);
      digitalWrite(7, LOW);
      Serial.println("f");
    }
    else if (inputvalue == 'B') {
      digitalWrite(10, LOW);
      digitalWrite(9, HIGH);
      digitalWrite(8, LOW);
      digitalWrite(7, HIGH);
      Serial.println("b");
    }

    else if (inputvalue == 'R') {
      digitalWrite(10, LOW);
      digitalWrite(9, HIGH);
      digitalWrite(8, HIGH);
      digitalWrite(7, LOW);
      Serial.println("r");
      delay(2000);
      digitalWrite(10, LOW);
      digitalWrite(9, LOW);
      digitalWrite(8, LOW);
      digitalWrite(7, LOW);
    }

    else if (inputvalue == 'L') {
      digitalWrite(10, HIGH);
      digitalWrite(9, LOW);
      digitalWrite(8, LOW);
      digitalWrite(7, HIGH);
      Serial.println("L");
      delay(2000);
      digitalWrite(10, LOW);
      digitalWrite(9, LOW);
      digitalWrite(8, LOW);
      digitalWrite(7, LOW);
    }

    else if (inputvalue == 'S') {
      digitalWrite(10, LOW);
      digitalWrite(9, LOW);
      digitalWrite(8, LOW);
      digitalWrite(7, LOW);
      Serial.println("s");
    }
    else if (inputvalue == 'A'){
      for (pos = 0; pos <= 45; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo1.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  for (pos = 45; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    servo1.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);    
    Serial.println("left servo");// waits 15 ms for the servo to reach the position
  }
    }
       else if (inputvalue == 'C'){
      for (pos = 0; pos <= 45; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo2.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  for (pos = 45; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    servo2.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);  
    Serial.println("left servo");// waits 15 ms for the servo to reach the position
  }
    }
      else if (inputvalue == 'H'){
      for (pos = 0; pos <= 45; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo3.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  for (pos = 45; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    servo3.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);  
    Serial.println("head");// waits 15 ms for the servo to reach the position
  }
    }
    
  }
 
}
