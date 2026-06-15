// ════════════════════════════════════════════════════════════════
// SMART WASTE CLASSIFIER — ARDUINO UNO MAIN SKETCH
// IT 171 Final Project
//
// HARDWARE:
//   Disc servo  (continuous rotation MG90S) → D9
//   Door servo  (positional SG90)           → D10
//   Ultrasonic TRIG                         → D6
//   Ultrasonic ECHO                         → D7
//   Buzzer                                  → D8
//   RGB
//   LCD I2C SDA                             → A4
//   LCD I2C SCL                             → A5
//   ESP32-CAM TX → Uno RX (SoftwareSerial) → D11
//   ESP32-CAM RX → Uno TX (SoftwareSerial) → D12
//
// POWER:
//   Both servos VCC + GND → Breadboard rail (5V 3A adapter)
//   Arduino powered       → Laptop USB
//   ESP32-CAM powered     → Same 5V adapter or laptop USB
//   ALL GNDs connected to breadboard – rail
//
// LIBRARIES NEEDED (install via Arduino IDE Library Manager):
//   LiquidCrystal_I2C  by Frank de Brabander
//   SoftwareSerial     (built-in, no install needed)
//   Servo              (built-in, no install needed)
// ════════════════════════════════════════════════════════════════
// ════════════════════════════════════════════════════════════════
// SMART WASTE CLASSIFIER — ARDUINO UNO MAIN SKETCH (RGB LIGHT UPDATED)
// ════════════════════════════════════════════════════════════════

#include <Servo.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ── PINS ─────────────────────────────────────────────────────────
#define DISC_PIN      9
#define DOOR_PIN      10
#define TRIG_PIN      6
#define ECHO_PIN      7
#define BUZZER_PIN    8

// RGB LED Pins (Replacing old separate LEDs)
#define RGB_R         3  
#define RGB_G         4  
#define RGB_B         5  

#define ESP_RX        11    // Uno receives from ESP32-CAM TX
#define ESP_TX        12    // Uno sends to ESP32-CAM RX

// ── DISC SERVO (continuous rotation) ─────────────────────────────
#define DISC_STOP     90
#define DISC_FWD      96    
#define DISC_BWD      84    

// ── STEP TIMING ──────────────────────────────────────────────────
#define STEP_MS       800   

// ── DOOR SERVO ───────────────────────────────────────────────────
#define DOOR_CLOSED   90
#define DOOR_OPEN     0

// ── TIMING & THRESHOLDS ──────────────────────────────────────────
#define DISC_SETTLE   800   
#define DOOR_HOLD     1500  
#define DOOR_SETTLE   400   
#define MIN_INTERVAL  3000  
#define DETECT_DIST   10    // cm
#define CONF_THRESHOLD 60   // %

LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo discServo;
Servo doorServo;
SoftwareSerial espSerial(ESP_RX, ESP_TX);

// ── STATE ────────────────────────────────────────────────────────
int  currentBin    = 0;     // 0=METAL 1=PAPER 2=PLASTIC
bool systemReady   = true;
unsigned long lastDropTime = 0;
String binNames[]  = { "METAL", "PAPER", "PLASTIC" };

// ════════════════════════════════════════════════════════════════
// SETUP
// ════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);

  discServo.attach(DISC_PIN);
  doorServo.attach(DOOR_PIN);
  discServo.write(DISC_STOP);
  doorServo.write(DOOR_CLOSED);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  digitalWrite(RGB_R, HIGH);
  digitalWrite(RGB_G, HIGH);
  digitalWrite(RGB_B, HIGH);

  // RGB Pins
  pinMode(RGB_R, OUTPUT);
  pinMode(RGB_G, OUTPUT);
  pinMode(RGB_B, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcdShow("WASTE CLASSIFIER", "Initializing...");
  
  // Test RGB and Buzzer
  setRGB(1, 1, 1); // White
  beep(1, 150);
  delay(500);
  setRGB(0, 0, 0); // Off

  lcdShow("SYSTEM READY", "Place waste item");
}

// ════════════════════════════════════════════════════════════════
// MAIN LOOP
// ════════════════════════════════════════════════════════════════
void loop() {
  if (!systemReady) return;
  if (millis() - lastDropTime < MIN_INTERVAL) return;

  if (!objectDetected()) return;

  systemReady = false;
  
  // 1. LIGHT UP THE TRAY
  setRGB(1, 1, 1); // Turn RGB White for the camera
  
  Serial.println("[TRIGGER] Object detected. Lighting tray...");
  lcdShow("Object detected", "Classifying...");
  beep(1, 80);

  // 2. WAIT FOR CAMERA EXPOSURE 
  // IMPORTANT: Give the ESP32-CAM 300ms to adjust to the new light level
  delay(5000); 

  // 3. ASK ESP32 TO CLASSIFY
  espSerial.println("CLASSIFY");

  String result = waitForResult(5000);

  if (result == "TIMEOUT") {
    handleError("TIMEOUT", "No response");
    return;
  }

  int commaIdx = result.indexOf(',');
  if (commaIdx == -1) {
    handleError("FORMAT ERR", "Bad ML result");
    return;
  }

  String cls  = result.substring(0, commaIdx);
  int    conf = result.substring(commaIdx + 1).toInt();
  cls.trim();

  if (conf < CONF_THRESHOLD) {
    setRGB(1, 0, 0); // Red for uncertainty
    lcdShow("UNCERTAIN", "Please retry");
    beep(2, 200);
    delay(2000);
    resetSystem();
    return;
  }

  int targetBin = getBinIndex(cls);
  if (targetBin == -1) {
    handleError("UNKNOWN", cls);
    return;
  }

  // SUCCESS: Show result and Sort
  setRGB(0, 1, 0); // Green for success
  lcdShow(cls + " " + String(conf) + "%", "Sorting...");
  
  dropInBin(targetBin, cls, conf);

  resetSystem();
}

// ════════════════════════════════════════════════════════════════
// DROP SEQUENCE
// ════════════════════════════════════════════════════════════════
void dropInBin(int targetBin, String cls, int conf) {
  rotateToBin(targetBin);
  delay(DISC_SETTLE);

  doorServo.write(DOOR_OPEN);
  delay(DOOR_HOLD);
  doorServo.write(DOOR_CLOSED);
  delay(DOOR_SETTLE);

  beep(1, 200);
  delay(1000);

  rotateToBin(0); // Return disc to home (Metal bin)
}

void rotateToBin(int targetBin) {
  if (targetBin == currentBin) return;
  int steps = (targetBin - currentBin + 3) % 3;
  for (int i = 0; i < steps; i++) {
    discServo.write(DISC_FWD);
    delay(STEP_MS);
    discServo.write(DISC_STOP);
    delay(500);
  }
  currentBin = targetBin;
}

// ════════════════════════════════════════════════════════════════
// HELPERS
// ════════════════════════════════════════════════════════════════
bool objectDetected() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float d = duration * 0.034 / 2.0;
  return (d > 0 && d < DETECT_DIST);
}

void setRGB(int r, int g, int b) {
  // Logic is inverted for Common Anode LEDs
  // LOW = ON, HIGH = OFF
  digitalWrite(RGB_R, r ? LOW : HIGH);
  digitalWrite(RGB_G, g ? LOW : HIGH);
  digitalWrite(RGB_B, b ? LOW : HIGH);
}
void handleError(String err1, String err2) {
  setRGB(1, 0, 0); // Red
  lcdShow(err1, err2);
  beep(3, 100);
  delay(2000);
  resetSystem();
}

void resetSystem() {
  setRGB(0, 0, 0); // Light off
  lcdShow("SYSTEM READY", "Place waste item");
  lastDropTime = millis();
  systemReady = true;
}

String waitForResult(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (espSerial.available()) {
      String res = espSerial.readStringUntil('\n');
      res.trim();
      return res;
    }
  }
  return "TIMEOUT";
}

int getBinIndex(String cls) {
  cls.toUpperCase();
  if (cls == "METAL")   return 0;
  if (cls == "PAPER")   return 1;
  if (cls == "PLASTIC") return 2;
  return -1;
}

void lcdShow(String l1, String l2) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(l1);
  lcd.setCursor(0, 1); lcd.print(l2);
}

void beep(int times, int durationMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(durationMs);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < times - 1) delay(100);
  }
}