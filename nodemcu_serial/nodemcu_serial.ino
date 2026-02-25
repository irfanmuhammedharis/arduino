#include<SoftwareSerial.h>
SoftwareSerial abc(13,15);
void setup(){
  abc.begin(9600);
}
void loop(){
  abc.write("1");
  delay(1000);
}
