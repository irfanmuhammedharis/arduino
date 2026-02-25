/* Receiver: Arduino Uno + SX1278 + 16x2 I2C LCD
   - LoRa pins: NSS D10, RST D9, DIO0 D2
   - LCD I2C (PCF8574) default address 0x27 (change if needed)
   - Receives CSV: pot,raw135,raw7,raw2,estNH3,estCO,estLPG
   - Displays rotating info two items at a time on the 16x2 LCD.
   - Also prints scientific names & values to Serial for clarity.
*/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const long LORA_FREQ = 433E6;
const int LORA_SS_PIN   = 10;
const int LORA_RST_PIN  = 9;
const int LORA_DIO0_PIN = 2;

LiquidCrystal_I2C lcd(0x27, 16, 2); // change 0x27 if your I2C address differs

// Helper: print a full 16-char line (pad/truncate)
void lcdPrintLine(int row, const char *text) {
  char buf[17];
  strncpy(buf, text, 16);
  buf[16] = '\0';
  int l = strlen(buf);
  for (int i=l;i<16;i++) buf[i] = ' ';
  lcd.setCursor(0,row);
  lcd.print(buf);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // LCD
  lcd.init();
  lcd.backlight();
  lcdPrintLine(0, "LoRa Receiver");
  lcdPrintLine(1, "Starting...");
  delay(600);
  lcd.clear();

  // LoRa
  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
  if (!LoRa.begin(LORA_FREQ)) {
    lcdPrintLine(0, "LoRa init failed");
    Serial.println("LoRa init failed!");
    while (1);
  }
  lcdPrintLine(0, "LoRa ready");
  delay(300);
  lcd.clear();
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  // Read incoming packet into a String
  String payload;
  payload.reserve(packetSize + 1);
  while (LoRa.available()) payload += (char)LoRa.read();

  Serial.print("Received raw: "); Serial.println(payload);

  // Expected CSV: pot,raw135,raw7,raw2,estNH3,estCO,estLPG
  // Split by commas
  int parts[7];
  for (int i=0;i<7;i++) parts[i] = -1;
  int idx = 0;
  int start = 0;
  while (idx < 7) {
    int comma = payload.indexOf(',', start);
    if (comma == -1) {
      // last part
      String part = payload.substring(start);
      parts[idx] = part.toInt();
      idx++;
      break;
    } else {
      String part = payload.substring(start, comma);
      parts[idx] = part.toInt();
      idx++;
      start = comma + 1;
    }
  }

  if (idx < 7) {
    // parse error
    lcdPrintLine(0, "Parse error");
    lcdPrintLine(1, payload.c_str());
    Serial.println("Parse error, packet incomplete.");
    return;
  }

  int pot = parts[0];
  int raw135 = parts[1];
  int raw7   = parts[2];
  int raw2   = parts[3];
  int estNH3 = parts[4];
  int estCO  = parts[5];
  int estLPG = parts[6];

  // Prepare display strings
  // We'll rotate two items per screen every 2.5s
  // Screen A: "NH3:xxx NH3 formula (printed to serial)"
  // But limited LCD width: we show gas name and value and a level letter (L/M/H)
  auto levelChar = [](int v)->char {
    if (v < 100) return 'L';
    if (v < 250) return 'M';
    return 'H';
  };

  char lineA1[17], lineA2[17], lineB1[17], lineB2[17];

  // Screen 1: CO and AIR (to match earlier layout)
  snprintf(lineA1, sizeof(lineA1), "CO:%3d%c AIR:%3d%c", estCO, levelChar(estCO), estNH3, levelChar(estNH3));
  // Screen 2: SMK (LPG) and POT
  snprintf(lineA2, sizeof(lineA2), "LPG:%3d%c POT:%3d", estLPG, levelChar(estLPG), pot);

  // Show Screen 1
  lcdPrintLine(0, lineA1);
  lcdPrintLine(1, lineA2);
  // Print scientific names to Serial with details
  Serial.println("--- Gas Estimates ---");
  Serial.print("Ammonia (NH3): "); Serial.print(estNH3); Serial.print(" /500  [");
  Serial.print((estNH3<100)?"Low":(estNH3<250)?"Moderate":"High"); Serial.println("]");

  Serial.print("Carbon monoxide (CO): "); Serial.print(estCO); Serial.print(" /500  [");
  Serial.print((estCO<100)?"Low":(estCO<250)?"Moderate":"High"); Serial.println("]");

  Serial.print("Propane (C3H8) - LPG: "); Serial.print(estLPG); Serial.print(" /500  [");
  Serial.print((estLPG<100)?"Low":(estLPG<250)?"Moderate":"High"); Serial.println("]");
  Serial.println("----------------------");

  delay(2500); // show this screen for 2.5s (adjust as desired)
}
