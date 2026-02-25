/*
 * ============================================================================
 * Project: Super Capacitor Charged Vehicle Controller
 * Version: Production v3.2 (Logic Update)
 * Board:   Arduino Uno R3
 * Author:  Senior Embedded Systems Lead
 * 
 * DESCRIPTION:
 * Controls a DC motor vehicle with logic-gated Super Capacitor injection.
 * 
 * NEW BEHAVIOR (v3.2):
 * 1. SC DISCHARGE (Boost) only activates if Motor is ON.
 * 2. REGEN (Braking) activates immediately when Motor turns OFF.
 * 
 * HARDWARE RESOURCES:
 * - I2C Bus (A4/A5): LCD (0x27) and MPU6050 (0x68)
 * - Pins D8/D9: Control Relays for Energy Management
 * 
 * LIBRARY DEPENDENCIES:
 * - Wire.h
 * - LiquidCrystal_I2C.h (https://github.com/johnrickman/LiquidCrystal_I2C)
 * - MPU6050_light.h (https://github.com/rfetick/MPU6050_light)
 * ============================================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MPU6050_light.h>
#include <avr/wdt.h>

// --- 1. CONFIGURATION & CONSTANTS ---

// Debugging
#define DEBUG_ENABLED       true
#define DEBUG_BAUD_RATE     115200

// Pin Definitions
const uint8_t PIN_I2C_SDA       = A4;
const uint8_t PIN_I2C_SCL       = A5;
const uint8_t PIN_BTN_MOTOR     = 2;  
const uint8_t PIN_BTN_MANUAL_SC = 3;  
const uint8_t PIN_BTN_ABORT     = 4;  
const uint8_t PIN_MOTOR_ENA     = 5;  
const uint8_t PIN_MOTOR_IN1     = 6;  
const uint8_t PIN_MOTOR_IN2     = 7;  
const uint8_t PIN_RELAY_DISCH   = 8;  // Boost
const uint8_t PIN_RELAY_REGEN   = 9;  // Brake/Charge
const uint8_t PIN_BUZZER        = 10; 

// Logic Thresholds
const float TILT_THRESHOLD      = 15.0; // Degrees X-axis
const unsigned long COUNTDOWN_MS = 5000; // Warning before boost
const unsigned long AUTO_OFF_MS  = 15000; // Max boost duration
const unsigned long REGEN_MS     = 4000;  // Braking duration

// Relay Active States (LOW = ON for standard modules)
const uint8_t RELAY_ON  = LOW;
const uint8_t RELAY_OFF = HIGH;

// --- 2. GLOBAL OBJECTS ---

LiquidCrystal_I2C lcd(0x27, 16, 2); 
MPU6050 mpu(Wire);

// System State Machine
enum State {
  IDLE, 
  MOTOR_RUN, 
  COUNTDOWN, 
  DISCHARGE, 
  REGEN, 
  FAULT
};
volatile State sysState = IDLE;

// Timing & State Variables
unsigned long timerStart = 0;
unsigned long lastUpdateLCD = 0;
unsigned long lastUpdateMPU = 0;
float tiltAngle = 0.0;
bool motorActive = false; // Tracks if motor logic thinks it's ON

// Input Handling Struct
struct Button {
  const uint8_t pin;
  bool state;
  bool lastState;
  unsigned long lastDebounce;
  bool triggered;
};

Button btnMotor = {PIN_BTN_MOTOR, 0, 0, 0, 0};
Button btnSC    = {PIN_BTN_MANUAL_SC, 0, 0, 0, 0};
Button btnAbort = {PIN_BTN_ABORT, 0, 0, 0, 0};

// --- 3. FUNCTION PROTOTYPES ---
void updateStateLogic(unsigned long now);
void readButton(Button &b);
void setMotor(int speed);
void abortSystem();
void startCountdown(unsigned long now);
void activateDischarge(unsigned long now);
void stopDischarge();
void startRegen(unsigned long now);
void stopRegen();
void refreshLCD();

// --- 4. SETUP ROUTINE ---
void setup() {
  wdt_disable();
  Serial.begin(DEBUG_BAUD_RATE);
  
  // Pin Config
  pinMode(PIN_RELAY_DISCH, OUTPUT);
  pinMode(PIN_RELAY_REGEN, OUTPUT);
  digitalWrite(PIN_RELAY_DISCH, RELAY_OFF);
  digitalWrite(PIN_RELAY_REGEN, RELAY_OFF);
  
  pinMode(PIN_MOTOR_ENA, OUTPUT);
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  digitalWrite(PIN_MOTOR_ENA, LOW);

  pinMode(PIN_BTN_MOTOR, INPUT);
  pinMode(PIN_BTN_MANUAL_SC, INPUT);
  pinMode(PIN_BTN_ABORT, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // I2C & LCD
  Wire.begin();
  Wire.setClock(400000);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print(F("SuperCap v3.2"));
  lcd.setCursor(0,1); lcd.print(F("Init Sensors..."));

  // MPU6050
  byte status = mpu.begin();
  if(status != 0) {
    lcd.setCursor(0,1); lcd.print(F("MPU Error!     "));
    while(1); 
  }
  
  Serial.println(F("[INIT] Calibrating..."));
  delay(1000);
  mpu.calcOffsets(true, true); 
  
  lcd.clear();
  lcd.print(F("System Ready"));
  tone(PIN_BUZZER, 2000, 100);
  wdt_enable(WDTO_2S);
}

// --- 5. MAIN LOOP ---
void loop() {
  wdt_reset();
  unsigned long now = millis();

  // 1. Inputs
  readButton(btnMotor);
  readButton(btnSC);
  readButton(btnAbort);

  // 2. Sensors (20Hz)
  if (now - lastUpdateMPU > 50) {
    mpu.update();
    tiltAngle = mpu.getAngleX(); 
    lastUpdateMPU = now;
  }

  // 3. Logic
  updateStateLogic(now);

  // 4. Display (4Hz)
  if (now - lastUpdateLCD > 250) {
    refreshLCD();
    lastUpdateLCD = now;
  }
}

// --- 6. STATE MACHINE LOGIC (UPDATED) ---
void updateStateLogic(unsigned long now) {
  
  // PRIORITY 1: EMERGENCY ABORT
  if (btnAbort.triggered) {
    abortSystem();
    return;
  }

  switch (sysState) {
    // --------------------------------------
    // IDLE: Waiting for start
    // --------------------------------------
    case IDLE:
      if (btnMotor.triggered) {
        motorActive = true;
        setMotor(200); // Start Drive
        sysState = MOTOR_RUN;
        Serial.println(F("[STATE] Drive Started"));
      }
      // NOTE: btnSC ignored here because Motor is OFF
      break;

    // --------------------------------------
    // MOTOR_RUN: Driving normal
    // --------------------------------------
    case MOTOR_RUN:
      // Condition A: Turn Off -> Go to REGEN
      if (btnMotor.triggered) {
        motorActive = false;
        setMotor(0); // Coast
        startRegen(now); // Trigger Braking/Regen
        return;
      }

      // Condition B: Boost Trigger (Tilt or Button)
      // Only allowed because we are in MOTOR_RUN
      if (abs(tiltAngle) > TILT_THRESHOLD || btnSC.triggered) {
        startCountdown(now);
      }
      break;

    // --------------------------------------
    // COUNTDOWN: Preparing to Boost
    // --------------------------------------
    case COUNTDOWN:
      {
        unsigned long elapsed = now - timerStart;
        
        // If user stops motor during countdown, cancel boost and Regen
        if (btnMotor.triggered) {
           motorActive = false;
           setMotor(0);
           sysState = IDLE; // Or go to Regen? Usually just stop safely.
           startRegen(now);
           return;
        }

        if (elapsed % 1000 < 100) tone(PIN_BUZZER, 1000); else noTone(PIN_BUZZER);
        
        if (elapsed >= COUNTDOWN_MS) {
          activateDischarge(now);
        }
      }
      break;

    // --------------------------------------
    // DISCHARGE: Boosting
    // --------------------------------------
    case DISCHARGE:
      {
        unsigned long elapsed = now - timerStart;
        
        // Condition A: User stops motor -> Kill Boost, Start Regen
        if (btnMotor.triggered) {
          stopDischarge();
          motorActive = false;
          setMotor(0);
          startRegen(now);
          return;
        }

        // Condition B: Time limit or Manual toggle -> Back to MOTOR_RUN
        if (elapsed >= AUTO_OFF_MS || btnSC.triggered) {
          stopDischarge();
          sysState = MOTOR_RUN; // Keep driving, just stop boost
        }
      }
      break;

    // --------------------------------------
    // REGEN: Braking / Recovering Energy
    // --------------------------------------
    case REGEN:
      {
        unsigned long elapsed = now - timerStart;
        
        // Regen ends automatically after timer
        if (elapsed >= REGEN_MS) {
          stopRegen();
          sysState = IDLE; // System is now parked/idle
          Serial.println(F("[STATE] Regen Complete -> IDLE"));
        }
        
        // Optional: If user presses Motor Button, cancel Regen and start Driving?
        // For safety, we usually force Regen to finish or require a full stop first.
        // We will allow override:
        if (btnMotor.triggered) {
          stopRegen();
          motorActive = true;
          setMotor(200);
          sysState = MOTOR_RUN;
        }
      }
      break;
      
    case FAULT:
      break;
  }
}

// --- 7. ACTUATOR CONTROL HELPERS ---

void startCountdown(unsigned long now) {
  sysState = COUNTDOWN;
  timerStart = now;
  Serial.println(F("[SEQ] Warning: Boost Imminent"));
}

void activateDischarge(unsigned long now) {
  sysState = DISCHARGE;
  timerStart = now;
  
  // Safety Interlock
  digitalWrite(PIN_RELAY_REGEN, RELAY_OFF); 
  delay(50); 
  
  digitalWrite(PIN_RELAY_DISCH, RELAY_ON);
  tone(PIN_BUZZER, 2000, 500); 
  Serial.println(F("[SEQ] BOOST ACTIVE"));
}

void stopDischarge() {
  digitalWrite(PIN_RELAY_DISCH, RELAY_OFF);
  Serial.println(F("[SEQ] Boost Deactivated"));
}

void startRegen(unsigned long now) {
  sysState = REGEN;
  timerStart = now;
  
  // Ensure Boost is OFF
  digitalWrite(PIN_RELAY_DISCH, RELAY_OFF);
  
  // Dead-time safety before engaging Regen Relay
  delay(50); 
  
  digitalWrite(PIN_RELAY_REGEN, RELAY_ON);
  Serial.println(F("[SEQ] REGEN/BRAKING ACTIVE"));
}

void stopRegen() {
  digitalWrite(PIN_RELAY_REGEN, RELAY_OFF);
  Serial.println(F("[SEQ] Regen OFF"));
}

void abortSystem() {
  digitalWrite(PIN_RELAY_DISCH, RELAY_OFF);
  digitalWrite(PIN_RELAY_REGEN, RELAY_OFF);
  setMotor(0);
  motorActive = false;
  sysState = IDLE;
  lcd.clear(); lcd.print(F("!! E-STOP !!"));
  tone(PIN_BUZZER, 500, 2000);
  delay(2000);
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
    case IDLE:      lcd.print(F("Sys: IDLE       ")); break;
    case MOTOR_RUN: lcd.print(F("Sys: DRIVING    ")); break;
    case COUNTDOWN: lcd.print(F("WARN: BOOST...  ")); break;
    case DISCHARGE: lcd.print(F(">> BOOSTING <<  ")); break;
    case REGEN:     lcd.print(F("<< BRAKING >>   ")); break;
    default:        lcd.print(F("Sys: WAIT       ")); break;
  }
  
  lcd.setCursor(0,1);
  if(sysState == COUNTDOWN) {
    int sec = 5 - (millis() - timerStart)/1000;
    lcd.print(F("T-Minus: ")); lcd.print(sec); lcd.print(F("s    "));
  } else if (sysState == REGEN) {
    int sec = 4 - (millis() - timerStart)/1000;
    lcd.print(F("Capturing: ")); lcd.print(sec); lcd.print(F("s  "));
  } else {
    lcd.print(F("Tilt: ")); lcd.print(tiltAngle, 1); lcd.print(char(223));
  }
}