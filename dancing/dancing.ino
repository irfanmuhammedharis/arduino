// Pin assignments for LEDs
const int ledPins[] = {2, 3, 4}; // LEDs connected to pins 2, 3, and 4
const int numLeds = 3;           // Total number of LEDs

void setup() {
  // Set all LED pins as outputs
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
}

void loop() {
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  delay(14000); //14 sec start
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  }
  delay(3000);
  blinkMultipleLeds(4000); 
  // 21 sec flicker
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);   
   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);  
   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);      // Wait for 2 seconds

  blinkMultipleLeds(5000); // Blink LEDs for 3 seconds
  //29 sec

  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);   
   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);  
         

  blinkMultipleLeds(5000); // Blink LEDs for 7 seconds

  //36 sec

  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);   
   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);  
   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);  
 blinkMultipleLeds(4000); 
 //43 sec
 for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);   
   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);  
   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);  

  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  delay(500);    
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  delay(500);            
   blinkMultipleLeds(12000); 
//59 sec

for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);   
   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);  
   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(500);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(500);  

    blinkMultipleLeds(12000); 
    //1.14
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  delay(500);   
  //1.17
   blinkMultipleLeds(12000); 
   //1.29

   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(300);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(300);   
   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(300);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(300);  
   for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  delay(300);     
    for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  } 
   delay(300);  
//
  blinkMultipleLeds(90000);
}

// Function to blink multiple LEDs randomly for a specified duration
void blinkMultipleLeds(unsigned long duration) {
  unsigned long startTime = millis(); // Record the start time

  while (millis() - startTime < duration) {
    for (int i = 0; i < numLeds; i++) {
      int randomState = random(0, 2);       // Randomly decide ON (1) or OFF (0)
      digitalWrite(ledPins[i], randomState); // Set LED to the random state
    }

    int randomDelay = random(100, 800); // Random delay between 100ms to 500ms
    delay(randomDelay);
  }

  // Ensure all LEDs are off after the duration
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }
}
