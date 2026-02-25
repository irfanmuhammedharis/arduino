

void setup() {
 


 pinMode(D4, OUTPUT); 
 pinMode(D5, OUTPUT); 


  
  Serial.begin(9600); // Starts the serial communication
}
void loop() {

    digitalWrite(D4,LOW);
    digitalWrite(D5,HIGH); 
    delay(1000);
    digitalWrite(D4,HIGH);
      digitalWrite(D5,LOW);
    delay(1000);
}
