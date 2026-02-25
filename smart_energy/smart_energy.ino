#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define pins
#define CURRENT_SENSOR_PIN A0

// Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables
float currentReading = 0.0; // Current sensor reading
float energyConsumption = 0.0; // Energy consumption in watt-hours

void setup() {
  Serial.begin(9600);
  lcd.init();                      // initialize the lcd
  lcd.backlight();                 // Turn on backlight
}

void loop() {
  // Read current sensor
  currentReading = analogRead(CURRENT_SENSOR_PIN);
  
  // Convert analog reading to current value (adjust calibration factor as needed)
  float current = map(currentReading, 0, 1023, 0, 5); // Assuming current sensor gives 0-5V output
  
  // Calculate energy consumption (you may need to adjust calculation based on sensor specifications)
  energyConsumption = current * 220; // Assuming voltage is 220V

  // Display energy consumption on LCD
  lcd.setCursor(0, 0);
  lcd.print("Energy (Wh): ");
  lcd.print(energyConsumption, 2); // Print with 2 decimal places
  
  // Send data via GSM (SIM800L module) - Example code, you need to adjust based on your GSM setup
  sendDataViaGSM(energyConsumption);
  
  delay(1000); // Adjust delay as needed
}

void sendDataViaGSM(float data) {
  // Example code to send data via GSM
  // You need to replace this with actual code for your SIM800L module setup
  // Initialize GSM module and connect to network
  // Send data over GSM network
  // Example:
  // Serial.println("AT+CMGF=1"); // Set SMS mode to text
  // delay(1000);
  // Serial.println("AT + CMGS = \"+1234567890\""); // Replace with your phone number
  // delay(1000);
  // Serial.println("Energy Consumption: " + String(data) + " Wh"); // Send energy consumption data
  // delay(1000);
  // Serial.println((char)26); // End AT command with Ctrl+Z
  // delay(1000);
}
