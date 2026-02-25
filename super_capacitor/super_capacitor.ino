/*
 * ============================================================================
 * Super Capacitor Charged Vehicle Controller - Production v3.1
 * ============================================================================
 * 
 * ARCHITECTURE: Shared I2C Bus (Standard Industry Practice)
 * ---------------------------------------------------------
 * On Arduino Uno, the I2C hardware is physically located on pins A4 & A5.
 * Both the LCD and MPU6050 MUST share these pins. They are distinguished
 * by their unique addresses:
 * - LCD Address:     0x27 (or 0x3F)
 * - MPU6050 Address: 0x68
 * 
 * PINOUT:
 * -------
 * [I2C BUS - SHARED]
 * - SDA: Pin A4 -> Connect to LCD SDA *AND* MPU6050 SDA
 * - SCL: Pin A5 -> Connect to LCD SCL *AND* MPU6050 SCL
 * 
 * [INPUTS - Active HIGH]
 * - D2: Motor Button
 * - D3: Manual SC Trigger
 * - D4: Abort / E-Stop
 * 
 * [OUTPUTS]
 * - D5, D6, D7: Motor Driver (L298N)
 * - D8: Relay 1 (SC Discharge)
 * - D9: Relay 2 (Regeneration)
 * - D10: Buzzer
 * 
 * ============================================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h> // Install via Library Manager
#include <MPU6050_light.h>     // Install via Library Manager
#include <avr/wdt.h>           // Watchdog Timer

// ============================================================================
// 1. CONFIGURATION
// ============================================================================
#define DEBUG_ENABLED       true
#define DEBUG_BAUD_RATE     115200
#define TEST_MODE_ENABLED   true

// I2C PINS (For Reference - Hardware Fixed on Uno)
const uint8_t PIN_I2C_SDA       = A4;
const uint8_t PIN_I2C_SCL       = A5;

// IO PINS
const uint8_t PIN_BTN_MOTOR     = 2;
const uint8_t PIN_BTN_MANUAL_SC = 3;
const uint8_t PIN_BTN_ABORT     = 4;
const uint8_t PIN_MOTOR_ENA     = 5;
const uint8_t PIN_MOTOR_IN1     = 6;
const uint8_t PIN_MOTOR_IN2     = 7;
const uint8_t PIN_RELAY_DISCH   = 8;
const uint8_t PIN_RELAY_REGEN   = 9;
const uint8_t PIN_BUZZER        = 10;

// SAFETY PARAMETERS
const float TILT_THRESHOLD      = 15.0; // Degrees
const float TILT_HYSTERESIS     = 2.0;
const unsigned long COUNTDOWN_MS = 5000;
const unsigned long AUTO_OFF_MS  = 15000;
const unsigned long REGEN_MS     = 3000;

// RELAY LOGIC (Most modules are Active LOW)
const uint8_t RELAY_ON  = LOW;
const uint8_t RELAY_OFF = HIGH;

// ============================================================================
// 2. GLOBAL OBJECTS
// ============================================================================
// LCD Address is usually 0x27. If screen is black, try 0x3F.
LiquidCrystal_I2C lcd(0x27, 16, 2); 
MPU6050 mpu(Wire);

// State Machine
enum State {
  IDLE, MOTOR_RUN, COUNTDOWN, DISCHARGE, REGEN, FAULT
};
volatile State sysState = IDLE;

// Variables
unsigned long timerStart = 0;
unsigned long lastUpdateLCD = 0;
unsigned long lastUpdateMPU = 0;
float tiltAngle = 0.0;
bool motorActive = false;
bool scActive = false;
bool regenActive = false;

// Button Debounce Struct
struct Button {
  const uint8_t pin;
  bool state;
  bool lastState;
  unsigned long lastDebounce;
  bool triggered;
} btnMotor = {PIN_BTN_MOTOR, 0, 0, 0, 0}, 
  btnSC = {PIN_BTN_MANUAL_SC, 0, 0, 0, 0}, 
  btnAbort = {PIN_BTN_ABORT, 0, 0, 0, 0};

// ============================================================================
// 3. SETUP
// ============================================================================
void setup() {
  wdt_disable();
  Serial.begin(DEBUG_BAUD_RATE);
  
  // -- Pin Config --
  pinMode(PIN_RELAY_DISCH, OUTPUT);
  pinMode(PIN_RELAY_REGEN, OUTPUT);
  digitalWrite(PIN_RELAY_DISCH, RELAY_OFF); // Force OFF
  digitalWrite(PIN_RELAY_REGEN, RELAY_OFF); // Force OFF
  
  pinMode(PIN_MOTOR_ENA, OUTPUT);
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  digitalWrite(PIN_MOTOR_ENA, LOW);

  pinMode(PIN_BTN_MOTOR, INPUT);
  pinMode(PIN_BTN_MANUAL_SC, INPUT);
  pinMode(PIN_BTN_ABORT, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // -- I2C Bus Init --
  Wire.begin(); // Joins I2C bus on A4/A5
  Wire.setClock(400000); // Fast Mode

  // -- LCD Init --
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print(F("SuperCap Ctrl"));
  lcd.setCursor(0,1); lcd.print(F("Init Sensors..."));

  // -- MPU6050 Init --
  byte status = mpu.begin();
  if(status != 0) {
    lcd.setCursor(0,1); lcd.print(F("MPU Error!     "));
    Serial.print(F("[ERR] MPU Connection Failed. Status: ")); Serial.println(status);
    while(1) { // Halt on error
      tone(PIN_BUZZER, 500, 200); delay(200);
    } 
  }
  
  Serial.println(F("[INIT] Calibrating MPU... Keep Level."));
  delay(1000);
  mpu.calcOffsets(true, true); // Auto-calibrate
  
  lcd.clear();
  lcd.print(F("System Ready"));
  tone(PIN_BUZZER, 2000, 100);
  wdt_enable(WDTO_2S); // Watchdog 2 seconds
}

// ============================================================================
// 4. MAIN LOOP
// ============================================================================
void loop() {
  wdt_reset();
  unsigned long now = millis();

  // -- Input Processing --
  readButton(btnMotor);
  readButton(btnSC);
  readButton(btnAbort);

  if (now - lastUpdateMPU > 50) {
    mpu.update();
    tiltAngle = mpu.getAngleX(); // Use X axis for tilt
    lastUpdateMPU = now;
    
    // Test Mode Injection
    #if TEST_MODE_ENABLED
    if(Serial.available()) {
      char c = Serial.read();
      if(c == '1') tiltAngle = 20.0; // Sim Tilt
      if(c == '0') tiltAngle = 0.0;
    }
    #endif
  }

  // -- Core Logic --
  updateStateLogic(now);

  // -- Output Processing --
  if (now - lastUpdateLCD > 250) {
    refreshLCD();
    lastUpdateLCD = now;
  }
}

// ============================================================================
// 5. STATE LOGIC
// ============================================================================
void updateStateLogic(unsigned long now) {
  
  // Global Safety: Abort
  if (btnAbort.triggered) {
    abortSystem();
    return;
  }

  // Global Trigger: Tilt or Button
  if ((abs(tiltAngle) > TILT_THRESHOLD || btnSC.triggered) && 
      sysState != COUNTDOWN && sysState != DISCHARGE && sysState != REGEN) {
    
    startCountdown(now);
    return;
  }

  switch (sysState) {
    // --------------------------------------
    case IDLE:
      if (btnMotor.triggered) {
        motorActive = true;
        setMotor(200); // Start Motor
        sysState = MOTOR_RUN;
      }
      break;

    // --------------------------------------
    case MOTOR_RUN:
      if (btnMotor.triggered) {
        motorActive = false;
        setMotor(0); // Stop Motor
        sysState = IDLE;
      }
      break;

    // --------------------------------------
    case COUNTDOWN:
      {
        unsigned long elapsed = now - timerStart;
        if (elapsed >= COUNTDOWN_MS) {
          activateDischarge(now);
        } else {
          // Beep every second
          if (elapsed % 1000 < 100) tone(PIN_BUZZER, 1000); else noTone(PIN_BUZZER);
        }
      }
      break;

    // --------------------------------------
    case DISCHARGE:
      {
        unsigned long elapsed = now - timerStart;
        // Auto-off or Manual Off
        if (elapsed >= AUTO_OFF_MS || btnSC.triggered) {
          stopDischarge();
          startRegen(now);
        }
      }
      break;

    // --------------------------------------
    case REGEN:
      {
        unsigned long elapsed = now - timerStart;
        if (elapsed >= REGEN_MS) {
          stopRegen();
          // Return to previous state
          sysState = motorActive ? MOTOR_RUN : IDLE;
        }
      }
      break;
  }
}

// ============================================================================
// 6. HELPERS
// ============================================================================

void startCountdown(unsigned long now) {
  sysState = COUNTDOWN;
  timerStart = now;
  Serial.println(F("[STATE] Countdown Started"));
}

void activateDischarge(unsigned long now) {
  sysState = DISCHARGE;
  timerStart = now;
  digitalWrite(PIN_RELAY_REGEN, RELAY_OFF); // Safety
  delay(50); // Hardware deadtime
  digitalWrite(PIN_RELAY_DISCH, RELAY_ON);
  scActive = true;
  tone(PIN_BUZZER, 2000, 500);
  Serial.println(F("[STATE] SC DISCHARGE ACTIVE"));
}

void stopDischarge() {
  digitalWrite(PIN_RELAY_DISCH, RELAY_OFF);
  scActive = false;
}

void startRegen(unsigned long now) {
  sysState = REGEN;
  timerStart = now;
  delay(50); // Safety gap
  digitalWrite(PIN_RELAY_REGEN, RELAY_ON);
  regenActive = true;
  Serial.println(F("[STATE] REGEN ACTIVE"));
}

void stopRegen() {
  digitalWrite(PIN_RELAY_REGEN, RELAY_OFF);
  regenActive = false;
}

void abortSystem() {
  digitalWrite(PIN_RELAY_DISCH, RELAY_OFF);
  digitalWrite(PIN_RELAY_REGEN, RELAY_OFF);
  scActive = false;
  regenActive = false;
  sysState = motorActive ? MOTOR_RUN : IDLE;
  lcd.clear(); lcd.print(F("!! ABORTED !!"));
  tone(PIN_BUZZER, 500, 1000);
  delay(1000);
  Serial.println(F("[ALERT] System Aborted"));
}

void setMotor(int speed) {
  if(speed > 0) {
    digitalWrite(PIN_MOTOR_IN1, HIGH);
    digitalWrite(PIN_MOTOR_IN2, LOW);
    analogWrite(PIN_MOTOR_ENA, speed);
  } else {
    digitalWrite(PIN_MOTOR_IN1, LOW);
    digitalWrite(PIN_MOTOR_IN2, LOW);
    analogWrite(PIN_MOTOR_ENA, 0);
  }
}

void readButton(Button &b) {
  bool reading = digitalRead(b.pin);
  b.triggered = false;
  if (reading != b.lastState) b.lastDebounce = millis();
  
  if ((millis() - b.lastDebounce) > 50) {
    if (reading != b.state) {
      b.state = reading;
      if (b.state == HIGH) b.triggered = true;
    }
  }
  b.lastState = reading;
}

void refreshLCD() {
  lcd.setCursor(0,0);
  switch(sysState) {
    case IDLE:      lcd.print(F("Mode: IDLE      ")); break;
    case MOTOR_RUN: lcd.print(F("Mode: DRIVING   ")); break;
    case COUNTDOWN: lcd.print(F("WARNING! SC...  ")); break;
    case DISCHARGE: lcd.print(F(">> BOOSTING <<  ")); break;
    case REGEN:     lcd.print(F("CHARGING...     ")); break;
  }
  
  lcd.setCursor(0,1);
  if(sysState == COUNTDOWN) {
    int sec = 5 - (millis() - timerStart)/1000;
    lcd.print(F("Activates in: ")); lcd.print(sec);
  } else {
    lcd.print(F("Tilt: ")); lcd.print(tiltAngle, 1); lcd.print(char(223));
  }
}