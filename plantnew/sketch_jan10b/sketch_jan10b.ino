/*
 * Absolute Position Control with TRUE Limit-Switch Homing
 * Arduino UNO + HW-040 + Relay Motor
 * HOME is set ONLY when limit switch is hit
 */

#define ENCODER_A 2
#define ENCODER_B 3

#define RELAY1 5
#define RELAY2 6
#define LIMIT_SWITCH 4

volatile long encoderCount = 0;
volatile uint8_t lastState = 0;

// Calibration
const float COUNTS_PER_CM = 241.0 / 38.0;   // ≈ 6.342

long targetCount = 0;
bool motorRunning = false;

void setup()
{
  Serial.begin(9600);

  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(LIMIT_SWITCH, INPUT_PULLUP);

  upper_motor_stop();

  lastState = (digitalRead(ENCODER_A) << 1) | digitalRead(ENCODER_B);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), encoderISR, CHANGE);

  Serial.println("Booting...");
  homeAxis();                 // AUTO HOME ON STARTUP
  Serial.println("Homed. Ready.");
}

void loop()
{
  // ---------- SERIAL COMMAND ----------
  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("HOME"))
    {
      Serial.println("Homing...");
      homeAxis();             // physical homing only
      Serial.println("HOME reached.");
    }
    else
    {
      float target_cm = cmd.toFloat();
      targetCount = target_cm * COUNTS_PER_CM;

      long currentCount;
      noInterrupts();
      currentCount = encoderCount;
      interrupts();

      long error = targetCount - currentCount;

      if (error != 0)
      {
        if (error > 0)
          upper_motor_forward();
        else
          upper_motor_backward();

        motorRunning = true;

        Serial.print("Moving to ");
        Serial.print(target_cm);
        Serial.println(" cm");
      }
    }
  }

  // ---------- POSITION CONTROL ----------
  if (motorRunning)
  {
    long currentCount;
    noInterrupts();
    currentCount = encoderCount;
    interrupts();

    long error = targetCount - currentCount;

    if (abs(error) <= 2)   // stop tolerance
    {
      upper_motor_stop();
      motorRunning = false;

      float pos_cm = currentCount / COUNTS_PER_CM;
      Serial.print("Position reached: ");
      Serial.print(pos_cm, 2);
      Serial.println(" cm");
    }
  }
}

// ================= TRUE HOMING =================
void homeAxis()
{
  upper_motor_backward();

  // MOVE UNTIL SWITCH IS PHYSICALLY HIT
  while (digitalRead(LIMIT_SWITCH) == HIGH)
  {
    delay(5);
  }

  upper_motor_stop();
  delay(200);

  // SET HOME ONLY HERE
  noInterrupts();
  encoderCount = 0;
  interrupts();
}

// ================= ENCODER ISR =================
void encoderISR()
{
  uint8_t state = (digitalRead(ENCODER_A) << 1) | digitalRead(ENCODER_B);
  uint8_t transition = (lastState << 2) | state;

  // Direction corrected (software-only)
  if (transition == 0b0001 || transition == 0b0111 ||
      transition == 0b1110 || transition == 0b1000)
    encoderCount--;

  else if (transition == 0b0010 || transition == 0b0100 ||
           transition == 0b1101 || transition == 0b1011)
    encoderCount++;

  lastState = state;
}

// ================= MOTOR FUNCTIONS =================
void upper_motor_forward()
{
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, HIGH);
}

void upper_motor_backward()
{
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, LOW);
}

void upper_motor_stop()
{
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
}