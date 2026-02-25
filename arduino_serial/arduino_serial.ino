int led = 13;
void setup() {
  // Start serial communication with a baud rate of 9600
  Serial.begin(9600); 
  pinMode(13,OUTPUT);
  while(!Serial);
  
}

void loop() {
  Serial.println("communication ");
  // Check if a message is available through serial communication
  if (Serial.available()) {
    int state = Serial.parseInt();
    if (state == 1){
      Serial.println("communication on");
      digitalWrite(13,HIGH);
    }
    else
    {
       digitalWrite(13,LOW);
       Serial.println("communication off");
    }
  }
}
    
