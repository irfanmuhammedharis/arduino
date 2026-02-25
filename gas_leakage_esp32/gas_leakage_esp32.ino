#include <WiFi.h>
#include <UniversalTelegramBot.h>
#include <HX711.h>
#include <Servo.h>

// WiFi Credentials
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

// Telegram Bot Credentials
const char* BOT_TOKEN = "Your_Telegram_Bot_Token";
const char* CHAT_ID = "Your_Chat_ID";

// Telegram Bot Object
WiFiClient client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Pins for MQ2 Sensor
#define MQ2_PIN 36

// Pins for HX711 Load Cell
#define LOADCELL_DOUT_PIN 19
#define LOADCELL_SCK_PIN 23

// Servo Motor Pin
#define SERVO_PIN 18

// Thresholds
#define GAS_THRESHOLD 300 // Adjust based on calibration
#define WEIGHT_EMPTY 5.0  // Minimum weight considered empty (kg)

// Objects
HX711 scale;
Servo servo;

// Variables
float weight = 0;
int gasValue = 0;

// Flags to avoid redundant messages
bool gasLeakNotified = false;
bool weightLowNotified = false;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  // Initialize HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(); // Default scale factor; calibrate this for accuracy
  scale.tare();      // Reset scale to 0

  // Initialize Servo
  servo.attach(SERVO_PIN);
  servo.write(0); // Initial position

  // Initialize MQ2
  pinMode(MQ2_PIN, INPUT);

  Serial.println("Setup complete!");
}

void loop() {
  // Read gas value
  gasValue = analogRead(MQ2_PIN);
  Serial.print("Gas Value: ");
  Serial.println(gasValue);

  // Read weight
  if (scale.is_ready()) {
    weight = scale.get_units(10); // Average over 10 readings
    Serial.print("Weight: ");
    Serial.print(weight, 2); // Two decimal places
    Serial.println(" kg");
  } else {
    Serial.println("HX711 not found.");
  }

  // Check for gas leakage
  if (gasValue > GAS_THRESHOLD) {
    if (!gasLeakNotified) {
      Serial.println("Gas leakage detected!");
      bot.sendMessage(CHAT_ID, "⚠️ Gas leakage detected! Please take immediate action.", "");
      gasLeakNotified = true; // Prevent duplicate notifications
    }
    servo.write(90); // Rotate servo to 90 degrees
    delay(500);      // Hold for half a second
    servo.write(0);  // Reset position
    delay(500);
  } else {
    gasLeakNotified = false; // Reset notification flag if no leak
  }

  // Check for low cylinder weight
  if (weight < WEIGHT_EMPTY) {
    if (!weightLowNotified) {
      Serial.println("Cylinder is empty or very low!");
      bot.sendMessage(CHAT_ID, "⚠️ Gas cylinder weight is very low! Consider replacing it.", "");
      weightLowNotified = true; // Prevent duplicate notifications
    }
  } else {
    weightLowNotified = false; // Reset notification flag if weight is normal
  }

  delay(1000); // Delay for 1 second
}
