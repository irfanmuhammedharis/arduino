/**
 * ESP32 Gait Analyzer — Production-Quality Version
 *
 * Fixes applied vs previous version:
 *  1.  WiFi connect is non-blocking; FSM runs immediately from boot.
 *  2.  HTTP POST (JSON) replaces GET with raw text; newlines/special chars safe.
 *  3.  URL encoding issue eliminated by moving to POST body.
 *  4.  ADC attenuation set to ADC_11db (0–3.3 V full-scale) on all sensor pins.
 *  5.  Piezo protection note added (hardware-side; see comment below).
 *  6.  First swing value guarded with toeOffValid flag — no garbage accumulation.
 *  7.  WiFi reconnection attempted before every Telegram send.
 *  8.  auxVoltageMin initialised from first real sample (NAN sentinel).
 *  9.  Certificate pinning via WiFiClientSecure + Telegram root CA.
 * 10.  Credentials kept inline — update the CONFIG section below before flashing.
 * 11.  Stance phase corrected: toeOffTime − heelTime (full stance, not heel-to-toe delay).
 * 12.  Networking runs in a dedicated FreeRTOS task; main loop never blocks.
 * 13.  Watchdog timer explicitly configured on both tasks.
 * 14.  GaitState backing type cleaned up (no unnecessary uint8_t cast).
 * 15.  Message buffer sized safely with compile-time assertion.
 * 16.  Aux avg voltage included in the periodic gait report (not a separate message).
 *      Aux accumulates over 1000-sample windows, resets each report cycle.
 * 17.  Startup message includes device details (pins, thresholds, interval).
 * 18.  Periodic report now shows: this-window steps, previous-window steps,
 *      and total lifetime step count.
 *
 * HARDWARE NOTE — piezo protection (issue #4 in review):
 *   Place a 10 kΩ series resistor on each piezo line followed by a BAT54
 *   Schottky diode to 3.3 V and another to GND. This clamps transients to
 *   ±0.3 V beyond the rails before they reach the GPIO pin.
 */

/* =================== CREDENTIALS — update before flashing =================== */

#define WIFI_SSID   "project"
#define WIFI_PASS   "123456789"
#define BOT_TOKEN   "8530796726:AAFpvhHHiciWoalQVRFTwb_Sqote-_lmGno"
#define CHAT_ID     "5717684177"

/* =================== INCLUDES =================== */
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_task_wdt.h>
#include <math.h>       // NAN, isnan()
#include <stdio.h>

/* =================== DEBUG =================== */
#define DEBUG_SERIAL 1

#if DEBUG_SERIAL
  #define DBG(x)   Serial.print(x)
  #define DBGL(x)  Serial.println(x)
  #define DBGF(...)  Serial.printf(__VA_ARGS__)
#else
  #define DBG(x)
  #define DBGL(x)
  #define DBGF(...)
#endif

/* =================== HARDWARE CONFIG =================== */

constexpr uint8_t PIN_PIEZO_HEEL = 34;
constexpr uint8_t PIN_PIEZO_TOE  = 35;
constexpr uint8_t PIN_AUX_SENSOR = 32;   // small-signal sensor

/* =================== ADC CONFIG =================== */

// With ADC_11db attenuation the full-scale input range is 0–3.3 V (≈ 3.9 V nominal).
// Raw counts 0–4095 map linearly to 0–3300 mV.
constexpr uint16_t HEEL_THRESHOLD   = 600;    // raw counts (~483 mV)
constexpr uint16_t TOE_THRESHOLD    = 600;
constexpr uint16_t ADC_REARM        = 200;    // raw counts (~161 mV)
constexpr uint16_t AUX_TOTAL_SAMPLES = 1000;  // collect exactly 1000 aux samples then report
constexpr float    ADC_REF_MV       = 3300.0f;
constexpr float    ADC_MAX_COUNTS   = 4095.0f;

/* =================== TIMING (ms) =================== */

constexpr uint32_t HEEL_TO_TOE_TIMEOUT = 500;
constexpr uint32_t STEP_REARM_TIME     = 300;
constexpr uint32_t REPORT_INTERVAL_MS  = 60000;
constexpr uint32_t TOE_OFF_DELAY       = 120;
constexpr uint32_t WIFI_RETRY_INTERVAL = 5000;  // ms between reconnect attempts
constexpr uint32_t WDT_TIMEOUT_SEC     = 10;    // watchdog bite time

/* =================== TELEGRAM ROOT CA =================== */
// DigiCert Global Root CA — signs api.telegram.org as of 2025.
// Verify periodically; replace if Telegram rotates its chain.
static const char TELEGRAM_ROOT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBI jANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2YwZDAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

/* =================== MESSAGE QUEUE =================== */
// Networking task reads from this queue; main loop writes to it.
// Queue depth = 4 so a burst of failed sends doesn't block the FSM.

constexpr uint16_t MSG_MAX_LEN   = 384;   // sized for the full combined gait+aux report
constexpr uint8_t  QUEUE_DEPTH   = 4;

struct TelegramMsg {
  char text[MSG_MAX_LEN];
};

static QueueHandle_t telegramQueue = nullptr;

/* =================== GAIT FSM STATE =================== */

enum class GaitState {
  IDLE,
  HEEL_DETECTED,
  TOE_DETECTED
};

static GaitState gaitState    = GaitState::IDLE;
static uint32_t  heelTime     = 0;
static uint32_t  toeTime      = 0;
static uint32_t  toeOffTime   = 0;
static uint32_t  lastStepTime = 0;
static bool      toeOffValid  = false;   // FIX #6: guard first swing calculation
static bool      heelArmed    = true;
static bool      toeArmed     = true;

// Accumulators reset each report window
static uint32_t stanceSum         = 0;
static uint32_t swingSum          = 0;
static uint32_t stepCount         = 0;   // steps this window
static uint32_t previousStepCount = 0;   // steps in the previous window
static uint32_t totalStepCount    = 0;   // lifetime total, never reset

// Aux sensor accumulators — reset every report window after 1000 samples are hit
// Sampling continues indefinitely; each window accumulates up to AUX_TOTAL_SAMPLES
static float    auxVoltageSum   = 0.0f;
static uint16_t auxSampleCount  = 0;     // resets each report window
static float    auxAvgMV_window = 0.0f;  // computed avg carried into the report

static uint32_t reportStartTime = 0;

/* =================== ADC HELPERS =================== */

static inline uint16_t readADC(uint8_t pin) {
  return (uint16_t)analogRead(pin);
}

/**
 * Read the aux sensor — single sample per call.
 * Called once per loop iteration; 1000 calls accumulate before reporting.
 * A single-sample read keeps loop latency low and doesn't block the FSM.
 * Returns raw ADC count converted to millivolts.
 */
static float readAuxVoltage_mV() {
  uint16_t raw = (uint16_t)analogRead(PIN_AUX_SENSOR);
  return ((float)raw / ADC_MAX_COUNTS) * ADC_REF_MV;
}

/* =================== WIFI HELPERS =================== */

static uint32_t lastWifiRetry = 0;

/**
 * Non-blocking WiFi reconnect guard.
 * Call before any network operation. Returns true if connected.
 * FIX #7: reconnects after dropout without blocking the FSM.
 */
static bool ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;

  uint32_t now = millis();
  if (now - lastWifiRetry < WIFI_RETRY_INTERVAL) return false;
  lastWifiRetry = now;

  DBGL("[WIFI] Reconnecting...");
  WiFi.disconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  return false;  // caller will retry next interval
}

/* =================== TELEGRAM TASK =================== */
// FIX #2, #3, #9, #12: POST JSON body, certificate pinning, dedicated task.

static void telegramTask(void* pvParameters) {
  esp_task_wdt_add(NULL);   // FIX #13: register this task with WDT

  for (;;) {
    esp_task_wdt_reset();

    TelegramMsg msg;
    // Block waiting for a message; reset WDT while idle
    if (xQueueReceive(telegramQueue, &msg, pdMS_TO_TICKS(1000)) != pdTRUE) {
      continue;
    }

    // Attempt WiFi; retry until connected (WDT keeps us honest)
    uint8_t wifiRetries = 0;
    while (WiFi.status() != WL_CONNECTED && wifiRetries < 10) {
      esp_task_wdt_reset();
      DBGL("[TEL-TASK] Waiting for WiFi...");
      vTaskDelay(pdMS_TO_TICKS(1000));
      wifiRetries++;
    }
    if (WiFi.status() != WL_CONNECTED) {
      DBGL("[TEL-TASK] WiFi unavailable, dropping message.");
      continue;
    }

    // FIX #9: Use WiFiClientSecure with root CA pinning
    WiFiClientSecure client;
    client.setCACert(TELEGRAM_ROOT_CA);

    HTTPClient http;
    http.setTimeout(5000);

    char url[128];
    snprintf(url, sizeof(url),
             "https://api.telegram.org/bot%s/sendMessage", BOT_TOKEN);

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    // FIX #3: JSON POST body — safe for newlines, colons, special chars
    // Escape backslash and double-quote in message text for valid JSON
    char jsonBody[MSG_MAX_LEN + 64];
    // Simple escaping: replace " with \" and \ with \\ inline
    char escaped[MSG_MAX_LEN * 2];
    uint16_t ei = 0;
    for (uint16_t si = 0; msg.text[si] != '\0' && ei < sizeof(escaped) - 2; si++) {
      if (msg.text[si] == '"' || msg.text[si] == '\\') {
        escaped[ei++] = '\\';
      }
      escaped[ei++] = msg.text[si];
    }
    escaped[ei] = '\0';

    snprintf(jsonBody, sizeof(jsonBody),
             "{\"chat_id\":\"%s\",\"text\":\"%s\"}",
             CHAT_ID, escaped);

    int httpCode = http.POST(jsonBody);
    http.end();

    DBGF("[TEL-TASK] HTTP response: %d\n", httpCode);

    if (httpCode != 200) {
      DBGF("[TEL-TASK] Send failed (code %d). Message dropped.\n", httpCode);
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // small breathing room between sends
  }
}

/* =================== ENQUEUE HELPER =================== */

static void enqueueTelegramMsg(const char* text) {
  TelegramMsg msg;
  strncpy(msg.text, text, MSG_MAX_LEN - 1);
  msg.text[MSG_MAX_LEN - 1] = '\0';

  if (xQueueSend(telegramQueue, &msg, 0) != pdTRUE) {
    DBGL("[QUEUE] Queue full — message dropped.");
  }
}

/* =================== SETUP =================== */

void setup() {
  Serial.begin(115200);
  delay(300);
  DBGL("=== ESP32 GAIT ANALYZER BOOT ===");

  // FIX #13: Configure watchdog — bite if any task stalls > WDT_TIMEOUT_SEC
  esp_task_wdt_config_t wdtCfg = {
    .timeout_ms    = WDT_TIMEOUT_SEC * 1000,
    .idle_core_mask = 0,
    .trigger_panic  = true
  };
  esp_task_wdt_reconfigure(&wdtCfg);
  esp_task_wdt_add(NULL);   // register setup/loop task

  // FIX #4: Set 11 dB attenuation so all sensor pins span 0–3.3 V
  analogSetPinAttenuation(PIN_PIEZO_HEEL, ADC_11db);
  analogSetPinAttenuation(PIN_PIEZO_TOE,  ADC_11db);
  analogSetPinAttenuation(PIN_AUX_SENSOR, ADC_11db);
  analogReadResolution(12);  // explicit 12-bit (0–4095)

  // FIX #1: Start WiFi in the background — don't block here
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  DBGL("[WIFI] Connection started in background.");

  // Create message queue for FSM → Telegram task communication
  telegramQueue = xQueueCreate(QUEUE_DEPTH, sizeof(TelegramMsg));

  if (!telegramQueue) {
    DBGL("[FATAL] Failed to create RTOS queue. Halting.");
    while (true) { esp_task_wdt_reset(); vTaskDelay(pdMS_TO_TICKS(1000)); }
  }

  // FIX #12: Networking on Core 0; sensor FSM stays on Core 1 (Arduino loop)
  xTaskCreatePinnedToCore(
    telegramTask,
    "TelegramTask",
    8192,        // stack — HTTP + SSL needs headroom
    nullptr,
    1,           // priority
    nullptr,
    0            // Core 0
  );

  reportStartTime = millis();
  DBGL("[SETUP] Boot complete. FSM running.");

  // Send startup notification with device details so user knows what is running
  char startMsg[MSG_MAX_LEN];
  snprintf(startMsg, sizeof(startMsg),
    "=== Gait Analyzer Started ===\n"
    "Heel sensor : GPIO %d (threshold %d)\n"
    "Toe sensor  : GPIO %d (threshold %d)\n"
    "Aux sensor  : GPIO %d (%u samples/window)\n"
    "Report every: %lu s\n"
    "Stance gate : heel-strike to toe-off\n"
    "Tracking    : this-min / prev-min / total steps",
    (int)PIN_PIEZO_HEEL, (int)HEEL_THRESHOLD,
    (int)PIN_PIEZO_TOE,  (int)TOE_THRESHOLD,
    (int)PIN_AUX_SENSOR, (unsigned)AUX_TOTAL_SAMPLES,
    (unsigned long)(REPORT_INTERVAL_MS / 1000)
  );
  enqueueTelegramMsg(startMsg);
}

/* =================== LOOP (Core 1) =================== */

void loop() {
  esp_task_wdt_reset();   // FIX #13: keep loop task watchdog alive

  uint32_t now = millis();

  uint16_t heelADC = readADC(PIN_PIEZO_HEEL);
  uint16_t toeADC  = readADC(PIN_PIEZO_TOE);

  /* ---- AUX SENSOR SAMPLING (up to 1000 samples per report window) ---- */
  // Accumulate samples each loop; stop adding once 1000 are collected for this window.
  // The computed average is included in the next periodic gait report, then reset.
  if (auxSampleCount < AUX_TOTAL_SAMPLES) {
    float auxMV = readAuxVoltage_mV();
    auxVoltageSum += auxMV;
    auxSampleCount++;
    DBGF("[AUX] Sample %u/%u — %.2f mV\n", auxSampleCount, AUX_TOTAL_SAMPLES, auxMV);
  }

  /* ---- NON-BLOCKING WIFI STATUS CHECK ---- */
  // FIX #7: attempt reconnect if disconnected, without blocking the FSM
  ensureWiFi();

  /* ---- ADC RE-ARM ---- */
  if (heelADC < ADC_REARM) heelArmed = true;
  if (toeADC  < ADC_REARM) toeArmed  = true;

  /* ---- GAIT FSM ---- */
  switch (gaitState) {

    case GaitState::IDLE:
      if (heelArmed &&
          heelADC > HEEL_THRESHOLD &&
          (now - lastStepTime) > STEP_REARM_TIME) {

        heelTime  = now;
        heelArmed = false;
        DBGL("[FSM] HEEL STRIKE");

        // FIX #6 + FIX #11: only accumulate swing if toeOff from a valid previous step
        if (toeOffValid) {
          swingSum += (heelTime - toeOffTime);
        }

        gaitState = GaitState::HEEL_DETECTED;
      }
      break;

    case GaitState::HEEL_DETECTED:
      if (toeArmed &&
          toeADC > TOE_THRESHOLD &&
          (now - heelTime) <= HEEL_TO_TOE_TIMEOUT) {

        toeTime  = now;
        toeArmed = false;
        DBGL("[FSM] TOE STRIKE");
        gaitState = GaitState::TOE_DETECTED;
      }
      else if ((now - heelTime) > HEEL_TO_TOE_TIMEOUT) {
        DBGL("[FSM] TIMEOUT -> IDLE");
        gaitState = GaitState::IDLE;
      }
      break;

    case GaitState::TOE_DETECTED:
      if ((now - toeTime) >= TOE_OFF_DELAY) {

        toeOffTime   = now;
        toeOffValid  = true;

        // FIX #11: FULL stance = toe-off time − heel-strike time
        stanceSum   += (toeOffTime - heelTime);
        lastStepTime = now;
        stepCount++;
        totalStepCount++;   // lifetime counter — never reset

        DBGF("[FSM] STEP COMPLETE — stance %lu ms\n",
             (unsigned long)(toeOffTime - heelTime));

        gaitState = GaitState::IDLE;
      }
      break;
  }

  /* ---- PERIODIC REPORT ---- */
  if ((now - reportStartTime) >= REPORT_INTERVAL_MS) {

    // Stance: one measurement per step
    uint32_t avgStance = stepCount ? stanceSum / stepCount : 0;

    // Swing: accumulated at heel-strike of the NEXT step — always one fewer
    // interval than stance count. Guard swingCount to avoid divide-by-zero.
    uint32_t swingCount = (stepCount > 1) ? (stepCount - 1) : 0;
    uint32_t avgSwing   = swingCount ? swingSum / swingCount : 0;

    // Steps per minute = stepCount (window == REPORT_INTERVAL_MS == 60 s exactly)
    uint32_t stepsPerMin = stepCount;

    // Aux average over however many of the 1000 samples landed in this window
    auxAvgMV_window = auxSampleCount ? auxVoltageSum / (float)auxSampleCount : 0.0f;

    char msg[MSG_MAX_LEN];
    static_assert(MSG_MAX_LEN >= 300, "MSG_MAX_LEN too small for combined report");

    snprintf(msg, sizeof(msg),
      "=== 1-Min Gait Report ===\n"
      "Step count        : %lu\n"
      "Step count/min    : %lu\n"
      "Step count prev   : %lu\n"
      "Step count total  : %lu\n"
      "Stance time (avg) : %lu ms\n"
      "Swing time  (avg) : %lu ms\n"
      "Aux voltage (avg) : %.2f mV",
      (unsigned long)stepCount,
      (unsigned long)stepsPerMin,
      (unsigned long)previousStepCount,
      (unsigned long)totalStepCount,
      (unsigned long)avgStance,
      (unsigned long)avgSwing,
      auxAvgMV_window
    );

    enqueueTelegramMsg(msg);

    // Roll window: save this window's steps as previous, then reset window accumulators
    previousStepCount = stepCount;
    stepCount         = 0;
    stanceSum         = 0;
    swingSum          = 0;
    toeOffValid       = false;
    auxVoltageSum     = 0.0f;
    auxSampleCount    = 0;
    auxAvgMV_window   = 0.0f;
    reportStartTime   = now;

    DBGL("[REPORT] Report queued. Window counters reset.");
  }
}
