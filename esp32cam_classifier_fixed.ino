// ════════════════════════════════════════════════════════════════
// SMART WASTE CLASSIFIER — ESP32-CAM SKETCH (FIXED)
// IT 171 Final Project
//
// WIRING TO ARDUINO UNO:
//   ESP32-CAM IO14 (TX) → Arduino D11
//   ESP32-CAM IO15 (RX) → Arduino D12
//   ESP32-CAM GND       → Common GND rail
//   ESP32-CAM 5V        → Breadboard + rail
//
// FLASHING STEPS:
//   1. Disconnect IO14 and IO15 wires from Arduino
//   2. Hold IO0 button on MB board
//   3. Click Upload in Arduino IDE
//   4. Release IO0 when you see "Connecting..."
//   5. Press RST button after upload finishes
//   6. Reconnect IO14 and IO15 wires to Arduino
//
// BOARD SETTINGS:
//   Board:        AI Thinker ESP32-CAM
//   Upload Speed: 115200
// ════════════════════════════════════════════════════════════════

#include "classified-capture-clone_inferencing.h"
#include "esp_camera.h"
#include "Arduino.h"

// ── CAMERA PINS (AI-Thinker ESP32-CAM) ───────────────────────────
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ── SERIAL PINS ───────────────────────────────────────────────────
// Serial  = USB to laptop (for debugging)
// Serial2 = to Arduino Uno via IO14/IO15
#define SERIAL2_RX  15   // IO15 receives from Arduino
#define SERIAL2_TX  14   // IO14 sends to Arduino

// ── SETTINGS ──────────────────────────────────────────────────────
#define CAPTURE_DELAY_MS  300

// ── GLOBALS ───────────────────────────────────────────────────────
static camera_fb_t *fb = NULL;

// ════════════════════════════════════════════════════════════════
// PIXEL EXTRACTION FOR EDGE IMPULSE
// ════════════════════════════════════════════════════════════════
static int getPixelData(size_t offset, size_t length, float *out_ptr) {
  if (fb == NULL) return -1;

  uint16_t *buf = (uint16_t *)fb->buf;
  size_t out_ptr_ix = 0;

  while (out_ptr_ix < length) {
    size_t pixel_ix = offset + out_ptr_ix;

    // Convert RGB565 to RGB888
    uint16_t pixel = buf[pixel_ix];
    uint8_t r = ((pixel >> 11) & 0x1F) << 3;
    uint8_t g = ((pixel >> 5)  & 0x3F) << 2;
    uint8_t b = ( pixel        & 0x1F) << 3;

    // Pack into single float as Edge Impulse expects
    out_ptr[out_ptr_ix] = (r << 16) + (g << 8) + b;
    out_ptr_ix++;
  }
  return 0;
}

// ════════════════════════════════════════════════════════════════
// CAMERA INIT
// ════════════════════════════════════════════════════════════════
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  // RGB565 format — needed for proper pixel extraction
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size   = FRAMESIZE_96X96;
  config.jpeg_quality = 10;
  config.fb_count     = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.print("Camera init failed: 0x");
    Serial.println(err, HEX);
    return false;
  }

  // Image quality settings
  sensor_t *s = esp_camera_sensor_get();
  s->set_gain_ctrl(s, 0);
  s->set_agc_gain(s, 5);
  s->set_exposure_ctrl(s, 0);
  s->set_aec_value(s, 300);
  s->set_sharpness(s, 2);
  s->set_wb_mode(s, 2);
  s->set_awb_gain(s, 1);
  s->set_special_effect(s, 0);
  s->set_saturation(s, 0);
  s->set_contrast(s, 1);
  s->set_hmirror(s, 0);
  s->set_vflip(s, 0);

  Serial.println("Camera initialized OK");
  return true;
}

// ════════════════════════════════════════════════════════════════
// CLASSIFY IMAGE
// ════════════════════════════════════════════════════════════════
void classifyImage() {
  // Capture frame
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    Serial2.println("UNKNOWN,0");
    return;
  }

  // Set up Edge Impulse signal
  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data     = &getPixelData;

  // Run classifier
  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);

  esp_camera_fb_return(fb);
  fb = NULL;

  if (err != EI_IMPULSE_OK) {
    Serial.print("Classifier failed: ");
    Serial.println(err);
    Serial2.println("UNKNOWN,0");
    return;
  }

  // Find top result
  int   topIndex = 0;
  float topValue = 0.0;
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > topValue) {
      topValue = result.classification[i].value;
      topIndex = i;
    }
  }

  int    confidence = (int)(topValue * 100);
  String className  = String(result.classification[topIndex].label);
  className.toUpperCase();

  // Print all results to laptop for debugging
  Serial.println("─── Classification Result ───");
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    Serial.print("  ");
    Serial.print(result.classification[i].label);
    Serial.print(": ");
    Serial.print((int)(result.classification[i].value * 100));
    Serial.println("%");
  }
  Serial.print("TOP: ");
  Serial.print(className);
  Serial.print(" ");
  Serial.print(confidence);
  Serial.println("%");
  Serial.println("─────────────────────────────");

  // Send result to Arduino Uno via Serial2
  // Format: CLASSNAME,CONFIDENCE
  Serial2.println(className + "," + String(confidence));
}

// ════════════════════════════════════════════════════════════════
// SETUP
// ════════════════════════════════════════════════════════════════
void setup() {
  // USB serial for laptop debugging
  Serial.begin(115200);
  delay(1000);

  // Serial2 for Arduino Uno communication
  // IO14 = TX (sends to Arduino D11)
  // IO15 = RX (receives from Arduino D12)
  Serial2.begin(9600, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);

  Serial.println("════════════════════════════════");
  Serial.println("  ESP32-CAM Waste Classifier");
  Serial.println("  IT 171 Final Project");
  Serial.println("════════════════════════════════");

  // Init camera
  if (!initCamera()) {
    Serial.println("CAMERA FAILED — check wiring");
    while (true) {
      digitalWrite(4, HIGH); delay(200);
      digitalWrite(4, LOW);  delay(200);
    }
  }

  // Warm up camera
  Serial.println("Warming up camera...");
  for (int i = 0; i < 5; i++) {
    camera_fb_t *warmup = esp_camera_fb_get();
    if (warmup) esp_camera_fb_return(warmup);
    delay(200);
  }

  Serial.println("ESP32-CAM READY");
  Serial.println("Waiting for CLASSIFY command...");
  Serial2.println("ESP32READY");
}

// ════════════════════════════════════════════════════════════════
// MAIN LOOP
// ════════════════════════════════════════════════════════════════
void loop() {
  // Wait for CLASSIFY command from Arduino Uno
  if (Serial2.available()) {
    String command = Serial2.readStringUntil('\n');
    command.trim();

    Serial.print("Received: ");
    Serial.println(command);

    if (command == "CLASSIFY") {
      Serial.println("Classifying...");
      delay(CAPTURE_DELAY_MS);
      classifyImage();
    }
  }
}
