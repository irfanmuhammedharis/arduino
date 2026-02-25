
#include <Servo.h>

int irPin = D4;
const int LM1 = D0;
const int LM2 = D1;
const int RM1 = D2;
const int RM2 = D3;
long duration;
int distance = 0;
int count = 0;
int turn = 0;
bool state = false;
int sensorvalue = 0;
#define trigPin  D6
#define echoPin  D5
Servo myservo;

void setup() {
  pinMode(LM1, OUTPUT);
  pinMode(LM2, OUTPUT);
  pinMode(RM1, OUTPUT);
  pinMode(RM2, OUTPUT);
  pinMode(irPin, INPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  myservo.attach(D7);
  Serial.begin(9600);
}

void loop() {
  myservo.write(90);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  Serial.print("Distance: ");
  Serial.println(distance);
  if (distance < 30 && distance > 5) {
    stops();
    delay(1000);
    Serial.println("Obstacle found in front");
    myservo.write(0);
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;
    Serial.print("Distance: ");
    Serial.println(distance);
    if (distance > 30) {

      turnLeft();
      Serial.println("turn left");
      delay(3000);
    }
    else {
      myservo.write(180);
      turnRight();
      Serial.println("turn right");
      delay(3000);
    }
  }

  else {
    ircount();
    myservo.write(90);
    moveForward();
  }
}

void ircount() {
  sensorvalue = digitalRead(D4);
  Serial.print("ir sensor");
  Serial.println(sensorvalue);

  if (sensorvalue == 1 && state == true) {
    state = false;

  }
  if (sensorvalue == 0 && state == false) {
    count++;
    state = true;
    Serial.print("Black color detected. Count: ");
    Serial.println(count);
  }
  if (count >= 10) {
    Serial.println("count reaches 10");
    stops();
    delay(1000);
    if (turn == 0 ) {
      Serial.println("turnning right");
      digitalWrite(LM1, LOW);
      digitalWrite(LM2, HIGH);
      digitalWrite(RM1, HIGH);
      digitalWrite(RM2, LOW);
      delay(3000);
      digitalWrite(LM1, HIGH);
      digitalWrite(LM2, LOW);
      digitalWrite(RM1, HIGH);
      digitalWrite(RM2, LOW);
      Serial.println("moving straight");
      delay(2000);
      digitalWrite(LM1, LOW);
      digitalWrite(LM2, HIGH);
      digitalWrite(RM1, HIGH);
      digitalWrite(RM2, LOW);
      Serial.println("turnning right");
      delay(3000);
      turn = 1;
      count = 0;
    }
    else {

      Serial.println("turnning left");
      digitalWrite(LM1, HIGH);
      digitalWrite(LM2, LOW);
      digitalWrite(RM1, LOW);
      digitalWrite(RM2, HIGH);
      delay(3000);
      digitalWrite(LM1, HIGH);
      digitalWrite(LM2, LOW);
      digitalWrite(RM1, HIGH);
      digitalWrite(RM2, LOW);
      Serial.println("moving straight");
      delay(2000);
      delay(2000);
      digitalWrite(LM1, HIGH);
      digitalWrite(LM2, LOW);
      digitalWrite(RM1, LOW);
      digitalWrite(RM2, HIGH);
      Serial.println("turnning left");
      delay(3000);
      turn = 0;
      count = 0;
    }

  }
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
void stops()
{
  digitalWrite(LM1, LOW);
  digitalWrite(LM2, LOW);
  digitalWrite(RM1, LOW);
  digitalWrite(RM2, LOW);
}
