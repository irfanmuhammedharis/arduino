
#define PWM1 6  // Direction for Left Motor
#define IN1 7  // Direction for Left Motor
#define PWM2 9  // PWM for Right Motor
#define IN2 8 // Direction for Right Motor


#define AUX1 10  // Auxiliary Motor 1
#define AUX2 11  // Auxiliary Motor 2

char command;

void setup() {
    Serial.begin(9600);
    pinMode(PWM1, OUTPUT);
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(PWM2, OUTPUT);
    pinMode(AUX1, OUTPUT);
    pinMode(AUX2, OUTPUT);
    stopMotors(); // Ensure motors are stopped initially
}

void loop() {
    if (Serial.available()) {
        command = Serial.read();
        Serial.println(command); // Debugging

        switch (command) {
            case 'F': moveForward(); break;
            case 'B': moveBackward(); break;
            case 'L': turnLeft(); break;
            case 'R': turnRight(); break;
            case 'S': stopMotors(); break;
            case '1': turnAuxMotor1On(); break;
            case '2': turnAuxMotor1Off(); break;
            case '3': turnAuxMotor2On(); break;
            case '4': turnAuxMotor2Off(); break;
        }
    }
}

void moveForward() {
    Serial.println("Moving Forward");
    digitalWrite(IN1, HIGH); 
    analogWrite(PWM1, 150); 
    digitalWrite(IN2, HIGH); 
    analogWrite(PWM2, 150); 
}

void moveBackward() {
    Serial.println("Moving Backward");
    digitalWrite(IN1, LOW); 
    analogWrite(PWM1, 150); 
    digitalWrite(IN2, LOW); 
    analogWrite(PWM2, 150);
}

void turnLeft() {
    Serial.println("Turning Left");
    digitalWrite(IN1, HIGH); 
    analogWrite(PWM1, 150); 
    digitalWrite(IN2, LOW); 
    analogWrite(PWM2, 150);
}

void turnRight() {
    Serial.println("Turning Right");
   digitalWrite(IN1, LOW); 
    analogWrite(PWM1, 150); 
    digitalWrite(IN2, HIGH); 
    analogWrite(PWM2, 150);
}

void stopMotors() {
    Serial.println("Stopping Motors");
    digitalWrite(IN1, LOW); 
    analogWrite(PWM1, 0); 
    digitalWrite(IN2, LOW); 
    analogWrite(PWM2, 0);
}

void turnAuxMotor1On() {
    Serial.println("Turning Auxiliary Motor 1 ON");
    digitalWrite(AUX1, HIGH);
}

void turnAuxMotor1Off() {
    Serial.println("Turning Auxiliary Motor 1 OFF");
    digitalWrite(AUX1, LOW);
}

void turnAuxMotor2On() {
    Serial.println("Turning Auxiliary Motor 2 ON");
    digitalWrite(AUX2, HIGH);
}

void turnAuxMotor2Off() {
    Serial.println("Turning Auxiliary Motor 2 OFF");
    digitalWrite(AUX2, LOW);
}
