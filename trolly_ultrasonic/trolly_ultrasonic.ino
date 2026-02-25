#define trigPinFront 9    // Trigger pin for front sensor
#define echoPinFront 10   // Echo pin for front sensor
#define trigPinLeft 7     // Trigger pin for left sensor
#define echoPinLeft 8     // Echo pin for left sensor
#define trigPinRight 5    // Trigger pin for right sensor
#define echoPinRight 6    // Echo pin for right sensor

// Motor control pins (replace with actual motor driver pins)
#define rightMotorForward 3
#define rightMotorBackward 4
#define leftMotorForward 2
#define leftMotorBackward 1

const int minDistance = 20;  // Minimum distance to follow (in cm)
const int maxDistance = 50;  // Maximum distance to follow (in cm)

void setup() {
  pinMode(trigPinFront, OUTPUT);
  pinMode(echoPinFront, INPUT);
  pinMode(trigPinLeft, OUTPUT);
  pinMode(echoPinLeft, INPUT);
  pinMode(trigPinRight, OUTPUT);
  pinMode(echoPinRight, INPUT);

  // Set motor control pins as output
  pinMode(rightMotorForward, OUTPUT);
  pinMode(rightMotorBackward, OUTPUT);
  pinMode(leftMotorForward, OUTPUT);
  pinMode(leftMotorBackward, OUTPUT);

  Serial.begin(9600); // Optional for debugging
}

void loop() {
  long durationFront, durationLeft, durationRight;
  int distanceFront, distanceLeft, distanceRight;

  // Read front sensor distance
  distanceFront = getSensorDistance(trigPinFront, echoPinFront);

  // Read side sensor distances based on front sensor reading

    // If too close in front, read both sides for turning
    distanceLeft = getSensorDistance(trigPinLeft, echoPinLeft);
    distanceRight = getSensorDistance(trigPinRight, echoPinRight);

    // If within desired front distance, assume clear path and don't read sides
    distanceLeft = 0;
    distanceRight = 0;
  

  Serial.print("Front: ");
  Serial.print(distanceFront);
  Serial.print(" Left: ");
  Serial.print(distanceLeft);
  Serial.print(" Right: ");
  Serial.println(distanceRight);

  // Follow logic based on sensor readings
  if (distanceFront <= minDistance) {
    if (distanceLeft > distanceRight) {
      turnRight();  
      Serial.print(" Right: ");// Turn right if closer obstacle on left
    } else if (distanceRight > distanceLeft) {
      turnLeft(); 
      Serial.print(" leftturn ");// Turn left if closer obstacle on right
    } else {
      moveBackward(); 
      Serial.print(" back word ");// Move back if obstacles on both sides
    }
  } else if (distanceFront >= maxDistance) {
    moveForward(); 
    Serial.print(" forward");// Move forward if clear path
  } else {
    stopMotors(); 
    Serial.print(" stop ");// Stop if within desired front distance
  }
}

int getSensorDistance(int trigPin, int echoPin) {
  long duration;
  int distance;

  // Trigger ultrasonic sensor
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo pulse
  duration = pulseIn(echoPin, HIGH);

  // Calculate distance
  distance = duration * 0.034 / 2;

  return distance;
}

void moveForward() {
  // Set motor directions for forward movement
  digitalWrite(rightMotorForward, HIGH);
  digitalWrite(rightMotorBackward, LOW);
  digitalWrite(leftMotorForward, HIGH);
  digitalWrite(leftMotorBackward, LOW);
}

void moveBackward() {
  // Set motor directions for backward movement
  digitalWrite(rightMotorForward, LOW);
  digitalWrite(rightMotorBackward, HIGH);
  digitalWrite(leftMotorForward, LOW);
  digitalWrite(leftMotorBackward, HIGH);
}

void stopMotors() {
  // Stop both motors
  digitalWrite(rightMotorForward, LOW);
  digitalWrite(rightMotorBackward, LOW);
  digitalWrite(leftMotorForward, LOW);
  digitalWrite(leftMotorBackward, LOW);
}

void turnLeft() {
  // Set motor directions for left turn (adjust based on motor setup)
  digitalWrite(rightMotorForward, HIGH);
  digitalWrite(rightMotorBackward, LOW);
  digitalWrite(leftMotorForward, LOW);
  digitalWrite(leftMotorBackward, HIGH);
}
void turnRight(){
   digitalWrite(rightMotorForward, LOW);
  digitalWrite(rightMotorBackward, HIGH);
  digitalWrite(leftMotorForward, HIGH);
  digitalWrite(leftMotorBackward, LOW);

}
