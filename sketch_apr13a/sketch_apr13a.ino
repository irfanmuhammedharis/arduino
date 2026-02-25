const int trigPin = 9;//stop
const int echoPin = 10;

const int trigPin1 = 7;//forward
const int echoPin1 = 8;

const int trigPin2 = 5;//left
const int echoPin2 = 4;

const int trigPin3 = 3;//right
const int echoPin3 = 2;

// defines variables
long duration;
int distance;
long duration1;
int distance1;
long duration2;
int distance2;
long duration3;
int distance3;

void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input


  pinMode(trigPin1, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin1, INPUT); // Sets the echoPin as an Input


  pinMode(trigPin2, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin2, INPUT); // Sets the echoPin as an Input

    pinMode(trigPin3, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin3, INPUT); // Sets the echoPin as an Input




 pinMode(46, OUTPUT); 
 pinMode(48, OUTPUT); 
 pinMode(50, OUTPUT); 
 pinMode(52, OUTPUT); 


  
  Serial.begin(9600); // Starts the serial communication
}
void loop() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distancefront: ");
  Serial.println(distance);




   digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration1 = pulseIn(echoPin1, HIGH);
  // Calculating the distance
  distance1 = duration1 * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distanceback: ");
  Serial.println(distance1);




   digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration2 = pulseIn(echoPin2, HIGH);
  // Calculating the distance
  distance2 = duration2 * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance left: ");
  Serial.println(distance2);




  digitalWrite(trigPin3, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin3, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin3, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration3 = pulseIn(echoPin3, HIGH);
  // Calculating the distance
  distance3 = duration3 * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance right: ");
  Serial.println(distance3);



if(distance<10)

{
          digitalWrite(46,LOW);
    digitalWrite(48,LOW); 
    digitalWrite(50,LOW);
      digitalWrite(52,LOW);

}

 else if(distance1<10)
{
 digitalWrite(46,HIGH);
    digitalWrite(48,LOW); 
    digitalWrite(50,HIGH);
      digitalWrite(52,LOW);
        delay(1500);
        digitalWrite(46,LOW);
    digitalWrite(48,LOW); 
    digitalWrite(50,LOW);
      digitalWrite(52,LOW);
}

 else if(distance2<10)
{
    digitalWrite(46,HIGH);
    digitalWrite(48,LOW); 
    digitalWrite(50,LOW);
      digitalWrite(52,HIGH);
       delay(150);
        digitalWrite(46,LOW);
    digitalWrite(48,LOW); 
    digitalWrite(50,LOW);
      digitalWrite(52,LOW); 
  
}

else if(distance3<10)
{
    digitalWrite(46,LOW);
    digitalWrite(48,HIGH); 
    digitalWrite(50,HIGH);
      digitalWrite(52,LOW);
      delay(150);
        digitalWrite(46,LOW);
    digitalWrite(48,LOW); 
    digitalWrite(50,LOW);
      digitalWrite(52,LOW);
}






















}
