#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD address (you may need to adjust this depending on your module)
#define LCD_ADDRESS 0x27

// Set the LCD dimensions (16x2 for a standard 16x2 LCD)
#define LCD_COLUMNS 16
#define LCD_ROWS 2
#define CURRENT_SENSOR_PIN A0

// Create an instance of the LiquidCrystal_I2C class
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
// Create a SoftwareSerial object to communicate with the SIM800L module
SoftwareSerial SIM800L(7, 8); // RX, TX
 float currentReading = 0.0; // Current sensor reading
float energyConsumption = 0.0; // Energy consumption in watt-hours
void setup() {
 
   lcd.init();
  lcd.clear();         
  lcd.backlight();  
  // Initialize Serial communication for debugging
  Serial.begin(9600);
  
  // Initialize SoftwareSerial communication with the SIM800L module
  SIM800L.begin(9600);
  
  // Give time to SIM800L to initialize
  delay(2000);
  
  // Enable SMS text mode
  SIM800L.println("AT+CMGF=1");
  delay(1000);
}

void loop() {
currentReading = analogRead(CURRENT_SENSOR_PIN);
   float current = map(currentReading, 0, 255, 0, 5);
   energyConsumption = current * 220;
   lcd.setCursor(0, 0);
  lcd.print("Energy (Wh): ");
  lcd.print(energyConsumption, 2);
  // Send an SMS
  smsRcv();
  
  // Delay before sending the next SMS
  delay(5000); // 5 seconds
}

void sendSMS(float data) {
  
  Serial.println("AT+CMGF=1"); // Set SMS mode to text
  delay(1000);
  Serial.println("AT + CMGS = \"+1234567890\""); // Replace with your phone number
  delay(1000);
  Serial.println("Energy Consumption: " + String(data) + " Wh"); // Send energy consumption data
  delay(1000);
  Serial.println((char)26); // End AT command with Ctrl+Z
  delay(1000);
}
void smsRcv()
{
if(SIM800L.available() > 0) {
    char c = SIM800L.read();
    if(c == '+') {
      // Read the rest of the incoming message
      String message = "";
      while(SIM800L.available()) {
        message += SIM800L.read();
      }
      
      // Check if the message contains "hello"
      if(message.indexOf("bill") != -1) {
        // Send a reply
        sendSMS(energyConsumption);
      }
    }
  }
}
