
#define LM1 4
#define LM2 5
#define RM1 6
#define RM2 7
char x;
int trigPin = 9;
int echoPin = 10;
long duration, distance;

void setup()
{

  Serial.begin(9600);
  pinMode(LM1, OUTPUT);
  pinMode(LM2, OUTPUT);
  pinMode(RM1, OUTPUT);
  pinMode(RM2, OUTPUT);
  pinMode(trigPin, OUTPUT);         
  pinMode(echoPin, INPUT);  
}

void loop()
{
  if (Serial.available())
  {
    x = Serial.read();
    Serial.print(x);


    switch (x)
    {
      case '1':  Forward();   break;
      case '2':  Backward();  break;
      case '3':  Left();      break;
      case '4':  Right();     break;
      case '5':  Stop();      break;
      case '6':
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);        
      delayMicroseconds(10);
      duration = pulseIn(echoPin, HIGH);  
      distance = duration * 0.034 / 2;   
      Serial.println(distance);     
     
      if (distance > 31)
      {
        digitalWrite(RM2, HIGH);   // move forward
        digitalWrite(RM1, LOW);
        digitalWrite(LM2, HIGH);
        digitalWrite(LM1, LOW);
        delay(500);
      }
      else if(distance < 30)
      {
        digitalWrite(RM2, LOW);    //Stop
        digitalWrite(RM1, LOW);
        digitalWrite(LM2, LOW);
        digitalWrite(LM1, LOW);
        delay(500);
        digitalWrite(RM2, LOW);    //movebackword
        digitalWrite(RM1, HIGH);
        digitalWrite(LM2, LOW);
        digitalWrite(LM1, HIGH);
        delay(500);
        digitalWrite(RM2, LOW);    //Stop
        digitalWrite(RM1, LOW);
        digitalWrite(LM2, LOW);
        digitalWrite(LM1, LOW);
        delay(100);
        digitalWrite(RM2, HIGH);   //turn left
        digitalWrite(RM1, LOW);
        digitalWrite(LM1, LOW);
        digitalWrite(LM2, LOW);
        delay(500);
  }
  break;

    }



  }
}
void Forward()
{
  digitalWrite(LM1, HIGH);
  digitalWrite(LM2, LOW);
  digitalWrite(RM1, HIGH);
  digitalWrite(RM2, LOW);
}
void Backward()
{
  digitalWrite(LM1, LOW);
  digitalWrite(LM2, HIGH);
  digitalWrite(RM1, LOW);
  digitalWrite(RM2, HIGH);
}
void Right()
{
  digitalWrite(LM1, LOW);
  digitalWrite(LM2, LOW);
  digitalWrite(RM1, HIGH);
  digitalWrite(RM2, LOW);
}
void Left ()
{
  digitalWrite(LM1, HIGH);
  digitalWrite(LM2, LOW);
  digitalWrite(RM1, LOW);
  digitalWrite(RM2, LOW);
}
void Stop()
{
  digitalWrite(LM1, LOW);
  digitalWrite(LM2, LOW);
  digitalWrite(RM1, LOW);
  digitalWrite(RM2, LOW);
}
