#include <Arduino.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "secrets.h"

// AI Thinker ESP32-CAM pins.
constexpr int PWDN_GPIO_NUM = 32;
constexpr int RESET_GPIO_NUM = -1;
constexpr int XCLK_GPIO_NUM = 0;
constexpr int SIOD_GPIO_NUM = 26;
constexpr int SIOC_GPIO_NUM = 27;
constexpr int Y9_GPIO_NUM = 35;
constexpr int Y8_GPIO_NUM = 34;
constexpr int Y7_GPIO_NUM = 39;
constexpr int Y6_GPIO_NUM = 36;
constexpr int Y5_GPIO_NUM = 21;
constexpr int Y4_GPIO_NUM = 19;
constexpr int Y3_GPIO_NUM = 18;
constexpr int Y2_GPIO_NUM = 5;
constexpr int VSYNC_GPIO_NUM = 25;
constexpr int HREF_GPIO_NUM = 23;
constexpr int PCLK_GPIO_NUM = 22;

constexpr uint8_t TRIGGER_PIN = 2;
constexpr uint8_t FLASH_PIN = 4;
constexpr uint32_t CAPTURE_COOLDOWN_MS = 8000;
constexpr uint32_t WIFI_RETRY_MS = 15000;

httpd_handle_t webServer = nullptr;
String pendingCaption = "ALERT: Motion detected";
bool capturePending = false;
bool previousTriggerState = false;
uint32_t lastCaptureMs = 0;
uint32_t lastWifiAttemptMs = 0;

bool initializeCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = psramFound() ? 2 : 1;
  config.grab_mode =
      psramFound() ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera initialization failed");
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor != nullptr) {
    sensor->set_brightness(sensor, 1);
    sensor->set_whitebal(sensor, 1);
    sensor->set_awb_gain(sensor, 1);
    sensor->set_exposure_ctrl(sensor, 1);
    sensor->set_gain_ctrl(sensor, 1);
  }

  return true;
}

void beginWiFiConnection() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttemptMs = millis();
}

void serviceWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  if (millis() - lastWifiAttemptMs >= WIFI_RETRY_MS) {
    Serial.println("Retrying Wi-Fi connection");
    WiFi.disconnect();
    beginWiFiConnection();
  }
}

bool sendTelegramText(const String &message) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("api.telegram.org", 443)) return false;

  String escapedMessage = message;
  escapedMessage.replace("\\", "\\\\");
  escapedMessage.replace("\"", "\\\"");
  escapedMessage.replace("\n", "\\n");
  escapedMessage.replace("\r", "");

  String body =
      "{\"chat_id\":\"" + String(TELEGRAM_CHAT_ID) +
      "\",\"text\":\"" + escapedMessage + "\"}";

  client.println(
      "POST /bot" + String(TELEGRAM_BOT_TOKEN) +
      "/sendMessage HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(body.length()));
  client.println("Connection: close");
  client.println();
  client.print(body);

  const uint32_t timeoutStart = millis();
  while (!client.available() && millis() - timeoutStart < 5000) {
    delay(10);
  }

  const String response = client.readString();
  client.stop();
  return response.indexOf("\"ok\":true") >= 0;
}

bool sendTelegramPhoto(const String &caption) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Image delivery skipped because Wi-Fi is offline");
    return false;
  }

  digitalWrite(FLASH_PIN, HIGH);
  delay(200);
  camera_fb_t *frame = esp_camera_fb_get();
  digitalWrite(FLASH_PIN, LOW);

  if (frame == nullptr) {
    Serial.println("Camera capture failed");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("api.telegram.org", 443)) {
    esp_camera_fb_return(frame);
    return false;
  }

  const String boundary = "BuildingGuardBoundary";
  const String head =
      "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" +
      String(TELEGRAM_CHAT_ID) + "\r\n" +
      "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"caption\"\r\n\r\n" +
      caption + "\r\n" +
      "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"photo\"; "
      "filename=\"building-guard-alert.jpg\"\r\n"
      "Content-Type: image/jpeg\r\n\r\n";
  const String tail = "\r\n--" + boundary + "--\r\n";
  const size_t contentLength = head.length() + frame->len + tail.length();

  client.println(
      "POST /bot" + String(TELEGRAM_BOT_TOKEN) +
      "/sendPhoto HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println("Content-Length: " + String(contentLength));
  client.println("Connection: close");
  client.println();
  client.print(head);

  size_t sent = 0;
  while (sent < frame->len) {
    const size_t chunk = min(static_cast<size_t>(1024), frame->len - sent);
    client.write(frame->buf + sent, chunk);
    sent += chunk;
  }
  client.print(tail);

  const uint32_t timeoutStart = millis();
  while (!client.available() && millis() - timeoutStart < 8000) {
    delay(10);
  }

  const String response = client.readString();
  client.stop();
  esp_camera_fb_return(frame);

  const bool delivered = response.indexOf("\"ok\":true") >= 0;
  Serial.println(delivered ? "Telegram image sent" : "Telegram image failed");
  return delivered;
}

esp_err_t streamHandler(httpd_req_t *request) {
  httpd_resp_set_type(
      request, "multipart/x-mixed-replace;boundary=frame");

  while (true) {
    camera_fb_t *frame = esp_camera_fb_get();
    if (frame == nullptr) return ESP_FAIL;

    char header[96];
    const size_t headerLength = snprintf(
        header,
        sizeof(header),
        "--frame\r\nContent-Type: image/jpeg\r\n"
        "Content-Length: %u\r\n\r\n",
        static_cast<unsigned int>(frame->len));

    esp_err_t result =
        httpd_resp_send_chunk(request, header, headerLength);
    if (result == ESP_OK) {
      result = httpd_resp_send_chunk(
          request,
          reinterpret_cast<const char *>(frame->buf),
          frame->len);
    }
    if (result == ESP_OK) {
      result = httpd_resp_send_chunk(request, "\r\n", 2);
    }

    esp_camera_fb_return(frame);
    if (result != ESP_OK) return result;
  }
}

esp_err_t captureHandler(httpd_req_t *request) {
  pendingCaption = "Manual capture from Building Guard dashboard";
  capturePending = true;
  httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(request, "Capture requested", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t rootHandler(httpd_req_t *request) {
  const String ipAddress = WiFi.localIP().toString();
  const String page =
      "<!doctype html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>Building Guard</title><style>"
      "body{margin:0;font-family:system-ui;background:#111;color:#fff}"
      "header{background:#d95b25;padding:14px 16px}"
      "h1{font-size:18px;margin:0}p{font-size:12px;margin:4px 0 0}"
      "img{display:block;width:100%;background:#000}"
      "button{width:calc(100% - 32px);margin:16px;padding:14px;"
      "border:0;border-radius:8px;background:#d95b25;color:#fff;"
      "font-weight:700}</style></head><body>"
      "<header><h1>Building Guard</h1><p>Local surveillance stream</p></header>"
      "<img src='/stream' alt='Live camera stream'>"
      "<button onclick=\"fetch('/capture').then(()=>alert('Capture requested'))\">"
      "Capture and send to Telegram</button>"
      "<p style='padding:0 16px'>Local address: http://" +
      ipAddress + "</p></body></html>";

  httpd_resp_set_type(request, "text/html");
  httpd_resp_send(request, page.c_str(), page.length());
  return ESP_OK;
}

void startWebServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.max_uri_handlers = 8;

  httpd_uri_t rootRoute = {};
  rootRoute.uri = "/";
  rootRoute.method = HTTP_GET;
  rootRoute.handler = rootHandler;
  rootRoute.user_ctx = nullptr;

  httpd_uri_t streamRoute = {};
  streamRoute.uri = "/stream";
  streamRoute.method = HTTP_GET;
  streamRoute.handler = streamHandler;
  streamRoute.user_ctx = nullptr;

  httpd_uri_t captureRoute = {};
  captureRoute.uri = "/capture";
  captureRoute.method = HTTP_GET;
  captureRoute.handler = captureHandler;
  captureRoute.user_ctx = nullptr;

  if (httpd_start(&webServer, &config) == ESP_OK) {
    httpd_register_uri_handler(webServer, &rootRoute);
    httpd_register_uri_handler(webServer, &streamRoute);
    httpd_register_uri_handler(webServer, &captureRoute);
  }
}

void interpretControllerCommand(const String &command) {
  if (command == "MOTION") {
    pendingCaption = "ALERT: Motion detected";
    capturePending = true;
  } else if (command == "TAMPER") {
    pendingCaption = "TAMPER ALERT: Physical disturbance detected";
    capturePending = true;
  } else if (command == "COVERED") {
    pendingCaption = "TAMPER ALERT: Camera lens covered";
    capturePending = true;
  }
}

void serviceControllerLink() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    interpretControllerCommand(command);
  }

  const bool triggerState = digitalRead(TRIGGER_PIN) == HIGH;
  if (triggerState && !previousTriggerState) {
    capturePending = true;
  }
  previousTriggerState = triggerState;
}

void setup() {
  Serial.begin(115200);
  pinMode(FLASH_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT_PULLDOWN);
  digitalWrite(FLASH_PIN, LOW);

  if (!initializeCamera()) {
    delay(3000);
    ESP.restart();
  }

  beginWiFiConnection();
  const uint32_t connectionStart = millis();
  while (
      WiFi.status() != WL_CONNECTED &&
      millis() - connectionStart < 10000) {
    delay(100);
  }

  startWebServer();
  if (WiFi.status() == WL_CONNECTED) {
    sendTelegramText(
        "BUILDING GUARD ONLINE\n\nLocal stream: http://" +
        WiFi.localIP().toString());
  }
}

void loop() {
  serviceWiFi();
  serviceControllerLink();

  if (
      capturePending &&
      millis() - lastCaptureMs >= CAPTURE_COOLDOWN_MS) {
    capturePending = false;
    lastCaptureMs = millis();
    sendTelegramPhoto(pendingCaption);
    pendingCaption = "ALERT: Motion detected";
  }

  delay(10);
}
