#define ENA 9
#define IN1 8
#define IN2 7
#define ENB 6
#define IN3 5
#define IN4 4
#define seril_start 10
#define forward 2
#define back 3
#define right 11
#define left 12

void setup() {
  Serial.begin(9600);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(seril_start, INPUT);
  pinMode(forward, INPUT);
  pinMode(back, INPUT);
  pinMode(left, INPUT);
  pinMode(right, INPUT);
  
  // Add pulldown resistors for input pins
  digitalWrite(seril_start, LOW);
  digitalWrite(forward, LOW);
  digitalWrite(back, LOW);
  digitalWrite(left, LOW);
  digitalWrite(right, LOW);
  
  // Initialize motors to a stopped state
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  
  // Set motor speed to maximum
  analogWrite(ENA, 255);
  analogWrite(ENB, 255);
  
  // Debug message
  Serial.println("Setup completed, motors initialized");
}

void loop() {
  int startSwitchState = digitalRead(seril_start);
  int frontSwitchState = digitalRead(forward);
  int backSwitchState = digitalRead(back);
  int rightSwitchState = digitalRead(right);
  int leftSwitchState = digitalRead(left);
  
  // Print all switch states for debugging
  Serial.print("Switches: Start=");
  Serial.print(startSwitchState);
  Serial.print(" F=");
  Serial.print(frontSwitchState);
  Serial.print(" B=");
  Serial.print(backSwitchState);
  Serial.print(" L=");
  Serial.print(leftSwitchState);
  Serial.print(" R=");
  Serial.println(rightSwitchState);
  
  // Check if all switches are LOW for serial control
  bool allSwitchesLow = (frontSwitchState == LOW && 
                        backSwitchState == LOW &&
                        rightSwitchState == LOW && 
                        leftSwitchState == LOW);
  
  // Process serial commands only when all switches are LOW
  if (Serial.available() > 0 && allSwitchesLow) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    Serial.print("Serial command received: ");
    Serial.println(command);
    
    if (command == "FORWARD") {
      moveForward();
      Serial.println("Serial: moveForward");
    } else if (command == "LEFT") {
      turnLeft();
      Serial.println("Serial: turnLeft");
    } else if (command == "RIGHT") {
      turnRight();
      Serial.println("Serial: turnRight");
    } else if (command == "BACK") {
      moveBackward();
      Serial.println("Serial: moveBackward");
    } else if (command == "STOP") {
      stopMotors();
      Serial.println("Serial: stopMotors");
    }
  }
  
  // Physical switch control - with motor status checks
  if (frontSwitchState == HIGH) {
    moveForward();
    Serial.println("Switch: moveForward");
    // Debug motor pin states
    printMotorStates();
  } else if (backSwitchState == HIGH) {
    moveBackward();  // Changed from stopMotors to moveBackward
    Serial.println("Switch: moveBackward");
    printMotorStates();
  } else if (leftSwitchState == HIGH) {
    turnLeft();
    Serial.println("Switch: turnLeft");
    printMotorStates();
  } else if (rightSwitchState == HIGH) {
    turnRight();
    Serial.println("Switch: turnRight");
    printMotorStates();
  } else if (allSwitchesLow) {
    stopMotors();
    // Don't spam the serial monitor with stop messages
  }
  
  // Add a short delay to reduce serial output spam
  delay(100);
}

void printMotorStates() {
  Serial.print("Motor pins - IN1:");
  Serial.print(digitalRead(IN1));
  Serial.print(" IN2:");
  Serial.print(digitalRead(IN2));
  Serial.print(" IN3:");
  Serial.print(digitalRead(IN3));
  Serial.print(" IN4:");
  Serial.println(digitalRead(IN4));
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}