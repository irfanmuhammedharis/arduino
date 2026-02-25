#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myservo;

enum VehicleType { CAR, TRUCK };

const int slot1 = 7;
const int slot2 = 8;
const int slot3 = 9;
const int slot4 = 10;
const int IR1pin = 1;
const int IR2pin = 6;
const int IR3pin = 12;

void setup() {
    Serial.begin(9600);
    lcd.init();
    lcd.backlight();

    // Set input and output pins
    pinMode(IR1pin, INPUT);
    pinMode(IR2pin, INPUT);
    pinMode(IR3pin, INPUT);
    pinMode(slot1, INPUT);
    pinMode(slot2, INPUT);
    pinMode(slot3, INPUT);
    pinMode(slot4, INPUT);
    myservo.attach(13);
}

void loop() {
    int IR1value = digitalRead(IR1pin);
    int IR2value = digitalRead(IR2pin);
    int IR3value = digitalRead(IR3pin);

    if (IR1value == 0 && IR2value == 1) {
        openGate();
        Serial.println("gate open car");
        delay(2000);
        checkAvailability(CAR);
    } else if (IR1value == 1 && IR2value == 1) {
        openGate();
        Serial.println("gate open truck");
        delay(2000);
        checkAvailability(TRUCK);
    } else if (IR3value == 1) {
        openGate(); // Exit button
        Serial.println("gate open exit");
    }
}

void openGate() {
    myservo.write(90); // Open the gate
    delay(5000);        // Wait for 5 seconds
    myservo.write(0);    // Close the gate
}

void checkAvailability(VehicleType vehicleType) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Slot available:");

    switch (vehicleType) {
        case CAR:
            if (digitalRead(slot1) == 0) {
                lcd.setCursor(1, 0);
                lcd.print("Slot open 1");
                Serial.println("slot one open");
            } else if (digitalRead(slot2) == 0) {
                lcd.setCursor(1, 0);
                lcd.print("Slot open 2");
                Serial.println("slot 2 open");
            } else if (digitalRead(slot3) == 0) {
                lcd.setCursor(1, 0);
                lcd.print("Slot open 3");
                Serial.println("slot 3 open");
            } else if (digitalRead(slot4) == 0) {
                lcd.setCursor(1, 0);
                lcd.print("Slot open 4");
                Serial.println("slot 4 open");
            } else {
                lcd.print("Slot closed");
                Serial.println("slot closed");
            }
            break;

        case TRUCK:
            bool slot1Available = digitalRead(slot1) == 0;
            bool slot2Available = digitalRead(slot2) == 0;

            if (slot1Available && slot2Available) {
                lcd.setCursor(1, 0);
                lcd.print("Slot open 1,2");
            } else if (slot2Available && digitalRead(slot3) == 0) {
                lcd.setCursor(1, 0);
                lcd.print("Slot open 2,3");
            } else if (digitalRead(slot3) == 0 && digitalRead(slot4) == 0) {
                lcd.setCursor(1, 0);
                lcd.print("Slot open 3,4");
            } else {
                lcd.print("Slot closed");
            }
            break;
    }
}
