#include <Servo.h>

Servo servo;

void setup() {
  // Initialize serial communication:
  Serial.begin(9600);
  
  // Attach the servo to pin 9 (choose a valid PWM pin)
  servo.attach(9, 544, 2400);
}

void loop() {
  // Read the serial input:
  if (Serial.available() > 0) {
    int inByte = Serial.read();
    
    // Control servo position based on serial input:
    switch (inByte) {
      case 'a':
        servo.write(36);
        break;
      case 'h':
        servo.write(72);
        break;
      case 'c':
        servo.write(108);
        break;
      case 'd':
        servo.write(144);
        break;
      case 'e':
        servo.write(180);
        break;
      case 'f':
        servo.write(90);
        break;
    }
  }
}
