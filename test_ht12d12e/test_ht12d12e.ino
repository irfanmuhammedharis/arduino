int switchPin = 8;
int outputPin = 7;
void setup() {
 pinMode(switchPin,INPUT);
 pinMode(outputPin, OUTPUT);// put your setup code here, to run once:
Serial.begin(9600);
}

void loop(){
 digitalWrite(outputPin,HIGH);
 Serial.println("sending signal");
 if(digitalRead(switchPin)==HIGH)
 {
  Serial.println("receiving signal");
  delay(1000);
  }// put your main code here, to run repeatedly:
else{
  Serial.println("no signal");
};
}
