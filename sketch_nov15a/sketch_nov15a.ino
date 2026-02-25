#define led1 D0
#define led2 D1
#define buzzer D2
void setup() {
pinMode(led2,OUTPUT);
pinMode(led1,OUTPUT);// put your setup code here, to run once:
pinMode(buzzer,OUTPUT);
}

void loop() {
  digitalWrite(led1,HIGH);
  digitalWrite(led2,LOW);
  digitalWrite(buzzer,LOW);
  delay(15000);
   digitalWrite(led1,LOW);
  digitalWrite(led2,HIGH);
  digitalWrite(buzzer,HIGH);
  delay(10000);
  
  // put your main code here, to run repeatedly:

}
