
int led1=13;
int led2=12;
int ir=7;

void setup(){
  pinMode(led1,OUTPUT);
  pinMode(led2,OUTPUT);
  pinMode(ir,INPUT);
}
void loop(){
  int sensorvalue = digitalRead(ir);
    if(sensorvalue==1)
    {
      digitalWrite(led1,HIGH);
      delay(1000);
      digitalWrite(led1,LOW);
      delay(1000); 
     }
    else
      {
        
       digitalWrite(led1,LOW);
       
       digitalWrite(led2,HIGH);
        
    }
  }
  
