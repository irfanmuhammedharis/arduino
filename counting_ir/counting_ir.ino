
 int irPin=2;  
 int count=0; 
 int turn=0; 
 boolean state = true;  
 void setup()  
 {  
  Serial.begin(9600);  
 
  pinMode(irPin, INPUT);   
 }  
 void loop()  
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
    Serial.println("zig");
    count=0;
    turn=1;
    }
    if (count==10 && turn==1)
  {
    Serial.println("zigleft");
    turn=0;
    count=0;
    }
 }  
