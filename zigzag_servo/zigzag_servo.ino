
#include <Servo.h>  
#include <NewPing.h>      

const int LM1 = 3;
const int LM2 = 4;
const int RM1 = 5;
const int RM2 = 6;
int irPin=2;  
 int count=0; 
 int turn=0; 
 boolean state = true;  
#define TRIGGER_PIN  A1  
#define ECHO_PIN     A2  
#define MAX_DISTANCE 250 

Servo servo;  // Servo's name
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 

boolean goesForward = false;
int distance = 100;

void setup()
{
  // Set L298N Control Pins as Output
  pinMode(LM1, OUTPUT);
  pinMode(LM2, OUTPUT);
  pinMode(RM1, OUTPUT);
  pinMode(RM2, OUTPUT);
  pinMode(irPin, INPUT); 

  servo.attach(9);   // Attachs the servo on pin 10 to servo object.
  servo.write(115);   // Set at 115 degrees.
  delay(2000);              // Wait for 2s.
  distance = readPing();    // Get Ping Distance.
  delay(100);               // Wait for 100ms.
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);
}

void loop()
{
  int distanceRight = 0;
  int distanceLeft = 0;
  delay(50);

  if (distance <= 40)
  {
    moveStop();
    delay(300);
    moveBackward();
    delay(500);
    moveStop();
    delay(300);
    distanceRight = lookRight();
    delay(300);
    distanceLeft = lookLeft();
    delay(300);

    if (distanceRight >= distanceLeft)
    {
      turnRight();
      delay(500);
      moveStop();
    }
    else
    {
      turnLeft();
      delay(500);
      moveStop();
    }

  }
  else
  {
    moveForward();
  }

  distance = readPing();
  counter();
}

int lookRight()     
{
  servo.write(50);
  delay(500);
  int distance = readPing();
  delay(100);
  servo.write(115);
  return distance;
}

int lookLeft()      
{
  servo.write(180);
  delay(500);
  int distance = readPing();
  delay(100);
  servo.write(115);
  return distance;
}

int readPing()    
{
  delay(100);                 
  int cm = sonar.ping_cm();   
  if (cm == 0)
  {
    cm = 250;
  }
  return cm;
}

void moveStop()       
{
  digitalWrite(LM1, LOW);
  digitalWrite(LM2, LOW);
  digitalWrite(RM1, LOW);
  digitalWrite(RM2, LOW);
}

void moveForward()    
{
  digitalWrite(LM1, HIGH);
  digitalWrite(LM2, LOW);
  digitalWrite(RM1, HIGH);
  digitalWrite(RM2, LOW);
}

void moveBackward()  
{
  digitalWrite(LM1, LOW);
  digitalWrite(LM2, HIGH);
  digitalWrite(RM1, LOW);
  digitalWrite(RM2, HIGH);
}

void turnRight()     
{
  digitalWrite(LM1, LOW);
  digitalWrite(LM2, HIGH);
  digitalWrite(RM1, HIGH);
  digitalWrite(RM2, LOW);
}

void turnLeft()       
{
  digitalWrite(LM1, HIGH);
  digitalWrite(LM2, LOW);
  digitalWrite(RM1, LOW);
  digitalWrite(RM2, HIGH);
}
void counter()
{
   if (!digitalRead(irPin) && state){  
    count++;  
    state = false;  
    Serial.print("Count: ");  
    Serial.println(count);   
    delay(100);  
  }  
  if (digitalRead(irPin))  
  {  
    state = true;  
    delay(100);  
  }  
  if (count==10 && turn==0)
  {
    Serial.println("zigrgt");
    zigrgt();
    count=0;
    turn=1;
    }
    if (count==10 && turn==1)
  {
    Serial.println("zigleft");
    ziglft();
    turn=0;
    count=0;
    }
}
void zigrgt()
{
  turnRight();
  delay(3000);
  moveForward();
  delay(2000);
  turnRight();
  delay(3000);
  moveForward();
  
}
void ziglft()
{
  turnLeft();
  delay(3000);
  moveForward();
  delay(2000);
  turnLeft();
  delay(3000);
  moveForward();
  
}
