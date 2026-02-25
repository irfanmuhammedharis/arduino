#include <Servo.h>

// Rotary Encoder Inputs
#define inputCLK 2
#define inputDT 3

// Servo signal pin
#define servo_pin 9

// Create a Servo object
Servo myservo;

// Variables for rotary encoder
int counter = 0;
int max_counter = 0;
int currentStateCLK;
int previousStateCLK;
unsigned long timer;
int t = 0;

void setup() 
{
  // Setup Serial Monitor
  Serial.begin (115200);

  // Set encoder pins as inputs  
  pinMode (inputCLK, INPUT);
  pinMode (inputDT, INPUT);

  // Attach servo on pin 9 to the servo object
  myservo.attach(servo_pin);

  // Read the initial state of inputCLK
  // Assign to previousStateCLK variable
  previousStateCLK = digitalRead(inputCLK);

  Serial.println();
  Serial.println("System initialized");
  Serial.println();
}

void loop() 
{
  // Read the current state of inputCLK
  currentStateCLK = digitalRead(inputCLK);

  // If the previous and the current state of the inputCLK are different then a pulse has occurred
  if (currentStateCLK != previousStateCLK)
  {
    if (t == 0)
    {
      t = 1;
      timer = millis();
    }

    // If the inputDT state is different than the inputCLK state then 
    // the encoder is rotating counterclockwise
    if (digitalRead(inputDT) != currentStateCLK)
    {
      // If the current counter value is more than 3, map it and save the maximum value
      if (counter > 3)
      {
        max_counter = map(counter, 3, 60, 0, 180);
        Serial.print("Max angle : ");
        Serial.println(max_counter);
      }

      // Move the servo to the maximum angle
      myservo.write(max_counter);

      // Reset the counter
      counter = 0;
    }
    else
    {
      // Increment the counter
      counter++;
    }

    // Update previousStateCLK with the current state
    previousStateCLK = currentStateCLK; 
  }

  // If the encoder angle becomes less than 3 degrees, reset the servo to 0 degrees
  if (counter < 3)
  {
    myservo.write(0);
  }

  // Check if 1 second has passed since the last rotation
  if (t == 1 && millis() - timer >= 1000) {
    t = 0; // Reset timer flag
  }

  delay(100);
}
