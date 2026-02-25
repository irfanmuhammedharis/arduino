#include <math.h>  // Required for sqrt() and log10()

// ----- Water Flow Sensor Variables -----
volatile uint32_t pulseCount = 0;
unsigned long previousMillis = 0;
const int flowSensorPin = 35;
const float calibrationFactor = 450.0; 

// ----- MAX4466 Sound Sensor Variables -----
// For MAX4466 with 5V supply, the output bias is ~2.5V.
// With a 3.3V ADC (12-bit, 4095 max), the bias corresponds to approximately 3100.
const int soundSensorPin = 32;
const int soundBias = 3100;  
// Adjust this calibration offset as needed after empirical calibration.
const float calibrationOffset = -40.0;

// ----- DF Robot Gravity Water Pressure Sensor Variables -----
// Updated pressure sensor settings with new calculation method:
const int pressureSensorPin = 33;
const float OffSet     = 0.377;  // Sensor's zero point offset 0.254
const float Scale      = 250.0;  // Conversion factor to kPa from (voltage - offset)

// ----- Interrupt Service Routine for Water Flow Sensor -----
void IRAM_ATTR pulseCounter() {
  pulseCount++;  
}

// ----- Function to Calculate Sound Level in dB -----
float getSoundLeveldB() {
  const int sampleCount = 100;
  long sumOfSquares = 0;
  
  for (int i = 0; i < sampleCount; i++) {
    int reading = analogRead(soundSensorPin);
    int deviation = reading - soundBias;
    sumOfSquares += (long)deviation * (long)deviation;
  }
  
  float rms = sqrt(sumOfSquares / (float)sampleCount);
  if (rms < 1.0) {
    rms = 1.0; // Avoid log(0)
  }
  
  float dB = 20.0 * log10(rms) + calibrationOffset;
  return dB;
}

void setup() {
  Serial.begin(115200);
  
  // Setup water flow sensor with interrupt
  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, FALLING);
  
  // Setup sensor pins for sound and pressure
  pinMode(soundSensorPin, INPUT);
  pinMode(pressureSensorPin, INPUT);
  
  previousMillis = millis();
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= 1000) {
    // Disable interrupts to safely capture and reset pulse count
    noInterrupts();
    uint32_t currentPulses = pulseCount;
    pulseCount = 0;
    interrupts();
    
    // ----- Water Flow Calculation -----
    float flowRate = (currentPulses / calibrationFactor) * 60.0;
    
    // ----- Sound Level Calculation -----
    float sounddB = getSoundLeveldB();

    // ----- Water Pressure Calculation (New Method) -----
    int pressureRaw = analogRead(pressureSensorPin);
    float voltage = pressureRaw * (3.3/ 4095.0);
    float pressure = (voltage - OffSet) * Scale;  // New calculation method
    
    // Print values: sound level (dB), pressure (kPa), and flow rate (based on calibration)
    Serial.print(sounddB);
    Serial.print(",");
    Serial.print(pressure);
    Serial.print(",");
    Serial.println(flowRate);
    
    previousMillis = currentMillis;
  }
}
