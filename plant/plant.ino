/*
 * Plant Picking Machine – FINAL VERIFIED CODE
 * Serial.write + Serial.read (NON-BLOCKING)
 * Arduino UNO
 */

/* ================= CALIBRATION ================= */
const float COUNTS_PER_CM = 241.0 / 38.0;
#define POSITION_TOLERANCE 2
#define HOME_TIMEOUT_MS 8000

/* ================= PINS ================= */
#define X_ENC_A   2
#define X_ENC_B   3
#define X_LIMIT   4
#define X_RELAY1  5
#define X_RELAY2  6

#define Y_RELAY1  7
#define Y_RELAY2  8
#define Y_ENC_A   18
#define Y_ENC_B   19
#define Y_LIMIT   11

#define START_BUTTON 12

/* ================= SERIAL RX BUFFER ================= */
char rxBuf[16];
uint8_t rxIndex = 0;

/* ================= VARIABLES ================= */
// ---- X ----
volatile long xCount = 0;
volatile uint8_t xLastState = 0;
long xTarget = 0;
bool xMoving = false;
int8_t xDir = 0;
bool leavingHome = false;

// ---- Y ----
volatile long yCount = 0;
volatile uint8_t yLastState = 0;
long yTarget = 0;
bool yMoving = false;
int8_t yDir = 0;

// ---- Positions ----
const float START_PICK_X_CM  = 24.0;
const float DROP_X_CM        = 30.0;
const float NEXT_STEP_CM     = 4.0;
const float FIRST_START_Y_CM = 10.0;

float currentPickXcm = START_PICK_X_CM;
bool firstStartDone = false;

// ---- Control ----
volatile bool startRequest = false;
volatile bool stopRequest  = false;

/* ================= STATE MACHINE ================= */
enum State {
  IDLE,
  MOVE_Y_FIRST_START,
  MOVE_TO_PICK,
  WAIT_PLACE,
  MOVE_TO_DROP,
  WAIT_NEXT,
  FAULT
};

State state = IDLE;

/* ================= SETUP ================= */
void setup()
{
  Serial.begin(9600);

  pinMode(X_ENC_A, INPUT_PULLUP);
  pinMode(X_ENC_B, INPUT_PULLUP);
  pinMode(X_LIMIT, INPUT_PULLUP);
  pinMode(X_RELAY1, OUTPUT);
  pinMode(X_RELAY2, OUTPUT);

  pinMode(Y_ENC_A, INPUT_PULLUP);
  pinMode(Y_ENC_B, INPUT_PULLUP);
  pinMode(Y_LIMIT, INPUT_PULLUP);
  pinMode(Y_RELAY1, OUTPUT);
  pinMode(Y_RELAY2, OUTPUT);

  pinMode(START_BUTTON, INPUT_PULLUP);

  xStop(); yStop();

  xLastState = (digitalRead(X_ENC_A) << 1) | digitalRead(X_ENC_B);
  yLastState = (digitalRead(Y_ENC_A) << 1) | digitalRead(Y_ENC_B);

  attachInterrupt(digitalPinToInterrupt(X_ENC_A), xEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(X_ENC_B), xEncoderISR, CHANGE);

  PCICR  |= (1 << PCIE0);
  PCMSK0 |= (1 << PCINT1) | (1 << PCINT2);

  Serial.println("BOOTING...");
  homeX();
  homeY();
  Serial.println("READY");
}

/* ================= LOOP ================= */
void loop()
{
  // ---- STOP HAS HIGHEST PRIORITY ----
  if (stopRequest)
  {
    stopRequest = false;
    xStop(); yStop();
    xMoving = yMoving = false;
    xDir = yDir = 0;
    state = IDLE;
    Serial.println("STOPPED");
    return;
  }

  handleSerial();

  if (xMoving) monitorX();
  if (yMoving) monitorY();

  switch (state)
  {
    case IDLE:
      if (digitalRead(START_BUTTON) == LOW)
      {
        delay(50);
        startRequest = true;
      }

      if (startRequest)
      {
        startRequest = false;

        if (!firstStartDone)
        {
          yTarget = FIRST_START_Y_CM * COUNTS_PER_CM;
          moveY();
          state = MOVE_Y_FIRST_START;
        }
        else
        {
          startPickMove();
        }
      }
      break;

    case MOVE_Y_FIRST_START:
      if (!yMoving)
      {
        firstStartDone = true;
        startPickMove();
      }
      break;

    case MOVE_TO_PICK:
      if (!xMoving)
      {
        Serial.write("PICK\n");   // ✅ protocol send
        state = WAIT_PLACE;
      }
      break;

    case MOVE_TO_DROP:
      if (!xMoving)
        state = WAIT_NEXT;
      break;

    case FAULT:
      xStop(); yStop();
      break;

    default: break;
  }
}

/* ================= SERIAL (NON-BLOCKING) ================= */
void handleSerial()
{
  while (Serial.available())
  {
    char c = Serial.read();

    if (c == '\n')
    {
      rxBuf[rxIndex] = '\0';
      rxIndex = 0;
      processCommand(rxBuf);
    }
    else if (rxIndex < sizeof(rxBuf) - 1)
    {
      rxBuf[rxIndex++] = c;
    }
  }
}

void processCommand(const char* cmd)
{
  if (strcmp(cmd, "STOP") == 0)
  {
    stopRequest = true;
  }
  else if (strcmp(cmd, "START") == 0 && state == IDLE)
  {
    startRequest = true;
  }
  else if (strcmp(cmd, "PLACE") == 0 &&
          (state == WAIT_PLACE || (state == MOVE_TO_PICK && !xMoving)))
  {
    xTarget = DROP_X_CM * COUNTS_PER_CM;
    moveX();
    state = MOVE_TO_DROP;
  }
  else if (strcmp(cmd, "NEXT") == 0 &&
          (state == WAIT_NEXT || (state == MOVE_TO_DROP && !xMoving)))
  {
    currentPickXcm -= NEXT_STEP_CM;
    xTarget = currentPickXcm * COUNTS_PER_CM;
    moveX();
    state = MOVE_TO_PICK;
  }
}

/* ================= HELPERS ================= */
void startPickMove()
{
  leavingHome = true;
  currentPickXcm = START_PICK_X_CM;
  xTarget = currentPickXcm * COUNTS_PER_CM;
  moveX();
  state = MOVE_TO_PICK;
}

/* ================= X AXIS ================= */
void moveX()
{
  long e = xTarget - xCount;
  if (e > 0) { xDir = +1; xForward(); }
  else if (e < 0) { xDir = -1; xBackward(); }
  else xDir = 0;
  xMoving = (e != 0);
}

void monitorX()
{
  if (!leavingHome && xDir == -1 && digitalRead(X_LIMIT) == LOW)
  {
    xStop();
    xCount = 0;
    xMoving = false;
    return;
  }

  if (abs(xTarget - xCount) <= POSITION_TOLERANCE)
  {
    xStop();
    xMoving = false;
    leavingHome = false;
  }
}

void homeX()
{
  xDir = -1;
  xBackward();
  unsigned long t = millis();
  while (digitalRead(X_LIMIT) == HIGH)
  {
    if (millis() - t > HOME_TIMEOUT_MS) return;
    delay(5);
  }
  xStop();
  xCount = 0;
  leavingHome = false;
}

void xEncoderISR()
{
  uint8_t s = (digitalRead(X_ENC_A) << 1) | digitalRead(X_ENC_B);
  uint8_t t = (xLastState << 2) | s;
  if (t == 1 || t == 7 || t == 14 || t == 8) xCount--;
  else if (t == 2 || t == 4 || t == 13 || t == 11) xCount++;
  xLastState = s;
}

void xForward()  { digitalWrite(X_RELAY1, LOW);  digitalWrite(X_RELAY2, HIGH); }
void xBackward() { digitalWrite(X_RELAY1, HIGH); digitalWrite(X_RELAY2, LOW); }
void xStop()     { digitalWrite(X_RELAY1, HIGH); digitalWrite(X_RELAY2, HIGH); }

/* ================= Y AXIS ================= */
void moveY()
{
  long e = yTarget - yCount;
  if (e > 0) { yDir = +1; yForward(); }
  else if (e < 0) { yDir = -1; yBackward(); }
  else yDir = 0;
  yMoving = (e != 0);
}

void monitorY()
{
  if (yDir == -1 && digitalRead(Y_LIMIT) == LOW)
  {
    yStop();
    yCount = 0;
    yMoving = false;
    return;
  }

  if (abs(yTarget - yCount) <= POSITION_TOLERANCE)
  {
    yStop();
    yMoving = false;
  }
}

void homeY()
{
  yDir = -1;
  yBackward();
  unsigned long t = millis();
  while (digitalRead(Y_LIMIT) == HIGH)
  {
    if (millis() - t > HOME_TIMEOUT_MS) return;
    delay(5);
  }
  yStop();
  yCount = 0;
}

ISR(PCINT0_vect)
{
  uint8_t s = (digitalRead(Y_ENC_A) << 1) | digitalRead(Y_ENC_B);
  uint8_t t = (yLastState << 2) | s;
  if (t == 1 || t == 7 || t == 14 || t == 8) yCount--;
  else if (t == 2 || t == 4 || t == 13 || t == 11) yCount++;
  yLastState = s;
}

void yForward()  { digitalWrite(Y_RELAY1, LOW);  digitalWrite(Y_RELAY2, HIGH); }
void yBackward() { digitalWrite(Y_RELAY1, HIGH); digitalWrite(Y_RELAY2, LOW); }
void yStop()     { digitalWrite(Y_RELAY1, HIGH); digitalWrite(Y_RELAY2, HIGH); }
