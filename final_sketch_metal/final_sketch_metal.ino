#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>
#include <ESP32Servo.h>

// The TinyGPSPlus object
TinyGPSPlus gps;  // Rx=Tx2,Tx=Rx2
Servo myservo;  // create servo object to control a servo

const char* ssid = "project1";
const char* password = "123456789";

// Initialize Telegram BOT
#define BOTtoken "6279751256:AAGqA60Z2ZMnS-ZSYkNQUePZt3rEebI66E4"  // your Bot Token (Get from Botfather)
#define CHAT_ID "1121687560"
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

const int proxi = 23; // Sensor connected

#define m_pin_1 32 // motor1a
#define m_pin_2 33 // motor1b
#define m_pin_3 25 // motor2a
#define m_pin_4 26 // motor2b

//#define radiation 27 // buzzer

int servoPin = 2;
int flag = 0;
 int irPin=19;  
 int count=0; 
 int turn=0; 
 boolean state = true;

const int trigPin = 5;  // TRIG pin of the ultrasonic sensor
const int echoPin = 18;  // ECHO pin of the ultrasonic sensor
// define sound speed in cm/uS
#define SOUND_SPEED 0.034

long duration;
float distance;
int pos = 90;  // Initialize servo position at 90 degrees

// Function prototypes
void fwd();
void bwd();
void rgt();
void lft();
void brk();
void sendLocation();

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  delay(2000);

  // Initialize ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(irPin, INPUT);
  pinMode(m_pin_1, OUTPUT); // motor1a
  pinMode(m_pin_2, OUTPUT); // motor1b
  pinMode(m_pin_3, OUTPUT); // motor2a
  pinMode(m_pin_4, OUTPUT); // motor2b
  pinMode(13, INPUT);// rad
  pinMode(proxi,INPUT);// metal

  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(servoPin, 500, 2400); // attaches the servo on pin 4 to the servo object
  myservo.write(90); // Start with servo at 90°

  // Connect to Wi-Fi network
  Serial.println();
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println(WiFi.localIP());

  bot.sendMessage(CHAT_ID, "Wrover started up", "");
}

void loop() {
  ultra();
  int rad = digitalRead(13);
  Serial.print("Radiation Level =");
  Serial.println(rad);
  if (rad == 0) {
    brk();
    bot.sendMessage(CHAT_ID, "Radiation is HIGH!!", "");
    Serial.println("Radiation is HIGH!!");
  }
  //  Radiation();
  while (Serial2.available() > 0)
    if (gps.encode(Serial2.read()))
      displayInfo();
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while (true);
  }
  int proxiState = digitalRead(proxi);
  if (proxiState == HIGH) { // Assuming HIGH state means object detected
    brk();
    bot.sendMessage(CHAT_ID, "Metal detected by sensor!!", "");
    Serial.println("Metal Detected by Sensor");
    sendLocation();
    delay(1000);
  }
  counter();
}
void zig()
{
  rgt();
  delay(3000);
  fwd();
  delay(3000);
  rgt ();
  delay(3000);
  brk();
  delay(500);
}
void zigleft()
{
   lft();
  delay(3000);
  fwd();
  delay(3000);
  lft ();
  delay(3000);
  brk();
  delay(500);
}
void ultra() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculate the distance
  distance = duration * SOUND_SPEED / 2;
  zig();
  Serial.print("Distance (cm): ");
  Serial.println(distance);

  if (pos == 90 && flag == 0) { // Servo is at initial position 90 degrees
    if (distance > 5 && distance < 10) {
      // Sweep from 90 to 0 degrees
      brk();
      delay(500);
      Serial.println("OBJECT IN FRONT");
      for (pos = 90; pos >= 0; pos -= 5) {
        myservo.write(pos);
        delay(100);

        // Measure distance during sweep
        measureDistance();
      }
      // Stay at 0 degrees if no object detected during sweep
      rgt();
      delay(1000);
      brk();
      delay(100);
      fwd();
      pos = 90;
      myservo.write(pos);
      flag = 1;
    }
  } else if (pos == 90 && flag == 1) { // Servo is at 0 degrees
    if (distance > 5 && distance < 10) {
      brk();
      delay(500);
      Serial.println("OBJECT IN RIGHT");
      //      myservo.write(90); // Return to 90 if object detected
      pos = 90;
      delay(500); // Wait for 2 seconds
      for (pos = 90; pos <= 180; pos += 5) {
        myservo.write(pos);
        delay(100);
        measureDistance();
      }
      lft();
      delay(2000);
      brk();
      delay(100);
      fwd();
      myservo.write(90); // Return to 90 if object detected
      pos = 90;
    }
  }  else if (pos == 180) { // Servo is at 180 degrees
    if (distance > 5 && distance < 10) {
      brk();
      delay(500);
      Serial.println("OBJECT IN LEFT");
      myservo.write(90); // Return to 90 if object detected
      pos = 90;
      rgt();
      delay(1000);
      brk();
      delay(100);
      fwd();
      delay(500); // Wait for 2 seconds
    }
  }
  // Small delay before the next loop
  delay(200);
}

void measureDistance() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculate the distance
  distance = duration * SOUND_SPEED / 2;
}

void fwd() {
  digitalWrite(m_pin_1, HIGH);
  digitalWrite(m_pin_2, LOW);
  digitalWrite(m_pin_3, HIGH);
  digitalWrite(m_pin_4, LOW);
  Serial.println("Forward");
}

void bwd() {
  digitalWrite(m_pin_1, LOW);
  digitalWrite(m_pin_2, HIGH);
  digitalWrite(m_pin_3, LOW);
  digitalWrite(m_pin_4, HIGH);
  Serial.println("Backward");
}

void rgt() {
  digitalWrite(m_pin_1, HIGH);
  digitalWrite(m_pin_2, LOW);
  digitalWrite(m_pin_3, LOW);
  digitalWrite(m_pin_4, HIGH);
  Serial.println("Right");
}

void lft() {
  digitalWrite(m_pin_1, LOW);
  digitalWrite(m_pin_2, HIGH);
  digitalWrite(m_pin_3, HIGH);
  digitalWrite(m_pin_4, LOW);
  Serial.println("Left");
}

void brk() {
  digitalWrite(m_pin_1, LOW);
  digitalWrite(m_pin_2, LOW);
  digitalWrite(m_pin_3, LOW);
  digitalWrite(m_pin_4, LOW);
  Serial.println("Stop");
}

void displayInfo() {
  Serial.print(F("Location: "));
  if (gps.location.isValid()) {
    Serial.print("Lat: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print("Lng: ");
    Serial.print(gps.location.lng(), 6);
    Serial.println();
  }
  else {
    Serial.println(F("INVALID"));
  }
}

void sendLocation() {
  if (gps.location.isValid()) {
    String locationMessage = "Metal detected at: \n";
    locationMessage += "Lat: ";
    locationMessage += String(gps.location.lat(), 6);
    locationMessage += ", Lng: ";
    locationMessage += String(gps.location.lng(), 6);
    bot.sendMessage(CHAT_ID, locationMessage, "");
  } else {
    bot.sendMessage(CHAT_ID, "Metal detected, but no valid GPS location.", "");
  }
}
  void counter(){
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
    zig();
    Serial.println("zig");
    count=0;
    turn=1;
    }
    if (count==10 && turn==1)
  {
    zigleft();
    Serial.println("zigleft");
    turn=0;
    count=0;
    }
 }  
  }
