#include <Arduino.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "secrets.h"

// ESP32-C3 Super Mini pins used by the submitted hardware.
constexpr uint8_t PIR_PIN = 5;
constexpr uint8_t VIBRATION_PIN = 2;
constexpr uint8_t LDR_PIN = 3;
constexpr uint8_t CAMERA_TRIGGER_PIN = 10;
constexpr uint8_t CAMERA_TX_PIN = 6;
constexpr uint8_t CAMERA_RX_PIN = 7;
constexpr uint8_t STATUS_LED_PIN = 0;

constexpr uint32_t CAMERA_BAUD = 115200;
constexpr uint32_t ALERT_COOLDOWN_MS = 10000;
constexpr uint32_t CAMERA_TRIGGER_MS = 500;
constexpr uint32_t SENSOR_SAMPLE_MS = 100;
constexpr uint32_t DEBUG_INTERVAL_MS = 2000;
constexpr uint32_t WIFI_RETRY_MS = 15000;
constexpr uint32_t VIBRATION_WINDOW_MS = 800;
constexpr uint32_t VIBRATION_DEBOUNCE_MS = 15;
constexpr uint8_t VIBRATION_MIN_HITS = 2;
constexpr float LDR_EMA_ALPHA = 0.02F;
constexpr float LDR_COVER_RATIO = 0.35F;
constexpr int LDR_DAYLIGHT_FLOOR = 150;
constexpr uint8_t LDR_CONFIRM_READS = 3;

enum class AlertType : uint8_t {
  Motion = 1,
  Vibration = 2,
  Covered = 3
};

HardwareSerial cameraSerial(1);
WiFiClientSecure telegramClient;
UniversalTelegramBot telegramBot(TELEGRAM_BOT_TOKEN, telegramClient);

volatile uint32_t vibrationHits = 0;
volatile uint32_t lastVibrationEdgeMs = 0;

uint32_t lastAlertMs[4] = {0, 0, 0, 0};
bool alertHasOccurred[4] = {false, false, false, false};
uint32_t vibrationWindowStartMs = 0;
uint32_t lastSensorSampleMs = 0;
uint32_t lastDebugMs = 0;
uint32_t lastWifiAttemptMs = 0;
uint32_t cameraTriggerStartedMs = 0;
uint32_t ledStepStartedMs = 0;

float ldrBaseline = 0.0F;
uint8_t ldrCoveredStreak = 0;
bool cameraTriggerActive = false;
bool wifiConnected = false;
uint8_t ledPulsesRemaining = 0;
uint16_t ledOnTimeMs = 0;
uint16_t ledOffTimeMs = 0;
bool ledIsOn = false;

void IRAM_ATTR onVibrationEdge() {
  const uint32_t now = millis();
  if (now - lastVibrationEdgeMs >= VIBRATION_DEBOUNCE_MS) {
    vibrationHits++;
    lastVibrationEdgeMs = now;
  }
}

void beginWiFiConnection() {
  Serial.println("Wi-Fi connection attempt started");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttemptMs = millis();
}

void serviceWiFi() {
  const bool connectedNow = WiFi.status() == WL_CONNECTED;

  if (connectedNow && !wifiConnected) {
    wifiConnected = true;
    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  } else if (!connectedNow && wifiConnected) {
    wifiConnected = false;
    Serial.println("Wi-Fi connection lost");
  }

  if (!connectedNow && millis() - lastWifiAttemptMs >= WIFI_RETRY_MS) {
    WiFi.disconnect();
    beginWiFiConnection();
  }
}

bool sendTelegramMessage(const String &message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Telegram message skipped because Wi-Fi is offline");
    return false;
  }

  const bool sent = telegramBot.sendMessage(TELEGRAM_CHAT_ID, message, "");
  Serial.println(sent ? "Telegram text sent" : "Telegram text failed");
  return sent;
}

const char *cameraCommand(AlertType type) {
  switch (type) {
    case AlertType::Motion: return "MOTION";
    case AlertType::Vibration: return "TAMPER";
    case AlertType::Covered: return "COVERED";
  }
  return "UNKNOWN";
}

void requestCameraCapture(AlertType type) {
  cameraSerial.println(cameraCommand(type));
  digitalWrite(CAMERA_TRIGGER_PIN, HIGH);
  cameraTriggerActive = true;
  cameraTriggerStartedMs = millis();
}

void serviceCameraTrigger() {
  if (cameraTriggerActive &&
      millis() - cameraTriggerStartedMs >= CAMERA_TRIGGER_MS) {
    digitalWrite(CAMERA_TRIGGER_PIN, LOW);
    cameraTriggerActive = false;
  }
}

void startLedPattern(AlertType type) {
  switch (type) {
    case AlertType::Motion:
      ledPulsesRemaining = 2;
      ledOnTimeMs = 100;
      ledOffTimeMs = 100;
      break;
    case AlertType::Vibration:
      ledPulsesRemaining = 4;
      ledOnTimeMs = 80;
      ledOffTimeMs = 80;
      break;
    case AlertType::Covered:
      ledPulsesRemaining = 1;
      ledOnTimeMs = 600;
      ledOffTimeMs = 100;
      break;
  }

  ledIsOn = true;
  digitalWrite(STATUS_LED_PIN, HIGH);
  ledStepStartedMs = millis();
}

void serviceLedPattern() {
  if (ledPulsesRemaining == 0) return;

  const uint32_t duration = ledIsOn ? ledOnTimeMs : ledOffTimeMs;
  if (millis() - ledStepStartedMs < duration) return;

  ledStepStartedMs = millis();
  if (ledIsOn) {
    digitalWrite(STATUS_LED_PIN, LOW);
    ledIsOn = false;
    ledPulsesRemaining--;
  } else if (ledPulsesRemaining > 0) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    ledIsOn = true;
  }
}

bool cooldownOver(AlertType type) {
  const uint8_t index = static_cast<uint8_t>(type);
  return !alertHasOccurred[index] ||
         millis() - lastAlertMs[index] >= ALERT_COOLDOWN_MS;
}

String alertText(AlertType type) {
  switch (type) {
    case AlertType::Motion:
      return "BUILDING GUARD ALERT\n\nMotion detected at the surveillance point.";
    case AlertType::Vibration:
      return "BUILDING GUARD ALERT\n\nTAMPER DETECTED\nPhysical disturbance detected on the camera.";
    case AlertType::Covered:
      return "BUILDING GUARD ALERT\n\nTAMPER DETECTED\nCamera lens appears blocked or covered.";
  }
  return "BUILDING GUARD ALERT";
}

void handleAlert(AlertType type) {
  if (!cooldownOver(type)) return;

  const uint8_t index = static_cast<uint8_t>(type);
  alertHasOccurred[index] = true;
  lastAlertMs[index] = millis();

  Serial.print("Alert confirmed: ");
  Serial.println(cameraCommand(type));

  requestCameraCapture(type);
  sendTelegramMessage(alertText(type));
  startLedPattern(type);
}

uint32_t consumeVibrationHits() {
  noInterrupts();
  const uint32_t hits = vibrationHits;
  vibrationHits = 0;
  interrupts();
  return hits;
}

void sampleSensors() {
  static uint32_t lastWindowHits = 0;
  const bool motionDetected = digitalRead(PIR_PIN) == HIGH;
  const int ldrValue = analogRead(LDR_PIN);
  bool vibrationConfirmed = false;

  if (millis() - vibrationWindowStartMs >= VIBRATION_WINDOW_MS) {
    lastWindowHits = consumeVibrationHits();
    vibrationConfirmed = lastWindowHits >= VIBRATION_MIN_HITS;
    vibrationWindowStartMs = millis();
  }

  const bool suddenLightDrop =
      ldrBaseline > LDR_DAYLIGHT_FLOOR &&
      ldrValue < ldrBaseline * LDR_COVER_RATIO;

  if (suddenLightDrop) {
    if (ldrCoveredStreak < LDR_CONFIRM_READS) ldrCoveredStreak++;
  } else {
    ldrCoveredStreak = 0;
    ldrBaseline =
        ldrBaseline * (1.0F - LDR_EMA_ALPHA) +
        ldrValue * LDR_EMA_ALPHA;
  }

  const bool cameraCovered = ldrCoveredStreak >= LDR_CONFIRM_READS;

  if (motionDetected) handleAlert(AlertType::Motion);
  if (vibrationConfirmed) handleAlert(AlertType::Vibration);
  if (cameraCovered) handleAlert(AlertType::Covered);

  if (millis() - lastDebugMs >= DEBUG_INTERVAL_MS) {
    Serial.printf(
        "PIR:%d VIB hits:%lu LDR:%d baseline:%.0f covered:%d WiFi:%s\n",
        motionDetected,
        static_cast<unsigned long>(lastWindowHits),
        ldrValue,
        ldrBaseline,
        cameraCovered,
        WiFi.status() == WL_CONNECTED ? "online" : "offline");
    lastDebugMs = millis();
  }
}

void calibrateLdr() {
  Serial.println("Calibrating LDR. Keep the lens and LDR uncovered.");
  delay(3000);

  uint32_t total = 0;
  constexpr uint8_t sampleCount = 10;
  for (uint8_t i = 0; i < sampleCount; i++) {
    total += analogRead(LDR_PIN);
    delay(100);
  }

  ldrBaseline = static_cast<float>(total) / sampleCount;
  Serial.printf("LDR baseline: %.0f\n", ldrBaseline);
}

void setup() {
  Serial.begin(115200);
  cameraSerial.begin(
      CAMERA_BAUD, SERIAL_8N1, CAMERA_RX_PIN, CAMERA_TX_PIN);

  pinMode(PIR_PIN, INPUT);
  pinMode(VIBRATION_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(CAMERA_TRIGGER_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(CAMERA_TRIGGER_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, LOW);

  attachInterrupt(
      digitalPinToInterrupt(VIBRATION_PIN),
      onVibrationEdge,
      CHANGE);

  telegramClient.setInsecure();
  calibrateLdr();
  beginWiFiConnection();

  vibrationWindowStartMs = millis();
  Serial.println("Building Guard controller monitoring started");
}

void loop() {
  serviceWiFi();
  serviceCameraTrigger();
  serviceLedPattern();

  if (millis() - lastSensorSampleMs >= SENSOR_SAMPLE_MS) {
    lastSensorSampleMs = millis();
    sampleSensors();
  }
}

