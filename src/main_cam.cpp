//
// ESP32‑CAM Stable Firmware with Watchdog + Stream Recovery
//

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_http_server.h"

// -------------------------
// WIFI CONFIG
// -------------------------
const char* ssid = "eir83427927";
const char* password = "3mU2cKFueD";

// -------------------------
// BACKEND URL
// -------------------------
String uploadUrl = "http://true-lion-mansion.loca.lt/upload_image";

// -------------------------
// CAMERA CONFIG
// -------------------------
#include "camera_pins.h"
camera_config_t camera_config;

// Forward declarations
esp_err_t stream_handler(httpd_req_t *req);
void startCameraServer();
esp_err_t snapshot_handler(httpd_req_t *req);
esp_err_t status_handler(httpd_req_t *req);

// -------------------------
// HEALTH MONITOR VARIABLES
// -------------------------
unsigned long lastFrameTimestamp = 0;
bool cameraHealthy = true;


// -------------------------
// CAMERA INIT WITH RETRY
// -------------------------
bool initCamera() {
  Serial.println("Initialising camera...");

  for (int i = 0; i < 5; i++) {
    esp_err_t err = esp_camera_init(&camera_config);
    if (err == ESP_OK) {
      Serial.println("Camera init OK");
      return true;
    }

    Serial.printf("Camera init failed (attempt %d)\n", i + 1);
    delay(500);
  }

  Serial.println("Camera failed after 5 attempts, restarting ESP...");
  delay(200);
  ESP.restart();
  return false;
}

// -------------------------
// SEND SNAPSHOT TO BACKEND
// -------------------------
void sendSnapshot() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Snapshot failed");
    return;
  }

  HTTPClient http;
  http.begin(uploadUrl);
  http.addHeader("Content-Type", "image/jpeg");

  int code = http.POST(fb->buf, fb->len);
  Serial.printf("Snapshot upload response: %d\n", code);

  esp_camera_fb_return(fb);
}

// -------------------------
// SNAPSHOT HTTP ENDPOINT
// -------------------------
esp_err_t snapshot_handler(httpd_req_t *req) {
  sendSnapshot();
  httpd_resp_sendstr(req, "Snapshot sent");
  return ESP_OK;
}

// -------------------------
// MJPEG STREAM HANDLER (FIXED)
// -------------------------
esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;

  static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace; boundary=frame";
  static const char* STREAM_BOUNDARY     = "--frame\r\n";
  static const char* STREAM_PART         = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

  httpd_resp_set_type(req, STREAM_CONTENT_TYPE);

  unsigned long lastFrameTime = millis();

  while (true) {
    // WATCHDOG: restart camera driver if no frames for 5 seconds
    if (millis() - lastFrameTime > 5000) {
      Serial.println("Stream stalled — restarting camera driver...");
      esp_camera_deinit();
      initCamera();
      return ESP_OK;
    }

    // SIMPLE DISCONNECT CHECK (socket invalid)
    int sock = httpd_req_to_sockfd(req);
    if (sock < 0) {
      Serial.println("Client disconnected");
      return ESP_OK;
    }

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed — restarting camera...");
      esp_camera_deinit();
      initCamera();
      return ESP_OK;
    }

    lastFrameTime       = millis();
    lastFrameTimestamp  = millis();
    cameraHealthy       = true;

    // BOUNDARY
    res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res != ESP_OK) {
      esp_camera_fb_return(fb);
      return res;
    }

    // HEADERS
    char header[64];
    size_t hlen = snprintf(header, sizeof(header), STREAM_PART, fb->len);
    res = httpd_resp_send_chunk(req, header, hlen);
    if (res != ESP_OK) {
      esp_camera_fb_return(fb);
      return res;
    }

    // JPEG DATA
    res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    if (res != ESP_OK) {
      return res;
    }

    // END OF FRAME
    res = httpd_resp_send_chunk(req, "\r\n", 2);
    if (res != ESP_OK) {
      return res;
    }

    // keep loop responsive without breaking timing
    yield();
  }

  return res;
}


// -------------------------
// STATUS ENDPOINT (SAFE)
// -------------------------
esp_err_t status_handler(httpd_req_t *req) {
  char buffer[256];

  snprintf(buffer, sizeof(buffer),
    "{ \"wifi\": \"%s\", \"ip\": \"%s\", \"uptime_ms\": %lu, \"last_frame_ms\": %lu, \"camera_ok\": %s }",
    (WiFi.status() == WL_CONNECTED ? "connected" : "disconnected"),
    WiFi.localIP().toString().c_str(),
    millis(),
    lastFrameTimestamp,
    (cameraHealthy ? "true" : "false")
  );

  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, buffer);
  return ESP_OK;
}

// -------------------------
// START CAMERA SERVER
// -------------------------
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;

  httpd_handle_t stream_httpd = NULL;
  httpd_start(&stream_httpd, &config);

  httpd_uri_t stream_uri = {
      .uri       = "/stream",
      .method    = HTTP_GET,
      .handler   = stream_handler,
      .user_ctx  = NULL
  };
  httpd_register_uri_handler(stream_httpd, &stream_uri);

  httpd_uri_t snapshot_uri = {
      .uri       = "/take_snapshot",
      .method    = HTTP_GET,
      .handler   = snapshot_handler,
      .user_ctx  = NULL
  };
  httpd_register_uri_handler(stream_httpd, &snapshot_uri);

  // STATUS ENDPOINT
  httpd_uri_t status_uri = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = status_handler,
    .user_ctx  = NULL
};
  httpd_register_uri_handler(stream_httpd, &status_uri);

  Serial.println("Camera server started");
}

// -------------------------
// SETUP
// -------------------------
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  // CAMERA CONFIG
  camera_config.ledc_channel = LEDC_CHANNEL_0;
  camera_config.ledc_timer = LEDC_TIMER_0;
  camera_config.pin_d0 = Y2_GPIO_NUM;
  camera_config.pin_d1 = Y3_GPIO_NUM;
  camera_config.pin_d2 = Y4_GPIO_NUM;
  camera_config.pin_d3 = Y5_GPIO_NUM;
  camera_config.pin_d4 = Y6_GPIO_NUM;
  camera_config.pin_d5 = Y7_GPIO_NUM;
  camera_config.pin_d6 = Y8_GPIO_NUM;
  camera_config.pin_d7 = Y9_GPIO_NUM;
  camera_config.pin_xclk = XCLK_GPIO_NUM;
  camera_config.pin_pclk = PCLK_GPIO_NUM;
  camera_config.pin_vsync = VSYNC_GPIO_NUM;
  camera_config.pin_href = HREF_GPIO_NUM;
  camera_config.pin_sscb_sda = SIOD_GPIO_NUM;
  camera_config.pin_sscb_scl = SIOC_GPIO_NUM;
  camera_config.pin_pwdn = PWDN_GPIO_NUM;
  camera_config.pin_reset = RESET_GPIO_NUM;
  camera_config.xclk_freq_hz = 20000000;
  camera_config.pixel_format = PIXFORMAT_JPEG;

  camera_config.frame_size = FRAMESIZE_QVGA;
  camera_config.jpeg_quality = 10;
  camera_config.fb_count = 2;

  initCamera();
  delay(2000);
  startCameraServer();
}

// -------------------------
// LOOP
// -------------------------
void loop() {
  // Nothing needed — watchdogs handle everything
}
