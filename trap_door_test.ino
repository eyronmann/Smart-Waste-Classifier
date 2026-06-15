// ════════════════════════════════════════════════════════
// ROTATING BIN TEST SKETCH
// Smart Waste Classifier — IT 171
//
// FOR CONTINUOUS ROTATION SERVO (the one that spins 360°)
//
// WIRING:
//   Servo signal (orange) → Arduino D9
//   Servo VCC   (red)     → Breadboard + rail (5V adapter)
//   Servo GND   (brown)   → Breadboard – rail (common GND)
//   Arduino GND           → Breadboard – rail (IMPORTANT)
//
// HOW TO USE:
//   1. Upload sketch
//   2. Open Serial Monitor at 9600 baud
//   3. Use commands below to calibrate timing
//   4. Note down STEP_MS value that works for your disc
//
// COMMANDS:
//   f → spin forward (free spin, press s to stop)
//   b → spin backward (free spin, press s to stop)
//   s → stop immediately
//   1 → go to Organic bin (home position)
//   2 → go to Paper bin (1 step forward)
//   3 → go to Plastic bin (2 steps forward)
//   h → return to home (Organic)
//   + → increase step duration by 50ms
//   - → decrease step duration by 50ms
//   ? → print current settings
//   t → run full auto test (all 3 bins in sequence)
// ════════════════════════════════════════════════════════

#include <Servo.h>

Servo disc;

// ── PINS
#define DISC_PIN   9

// ── CONTINUOUS ROTATION SPEEDS
// These values control direction and speed
// 90 = STOP · >90 = forward · <90 = backward
// Adjust if your servo doesn't stop at 90
#define STOP        90
#define FWD_SLOW    96    // forward slow (less likely to overshoot)
#define BWD_SLOW    84    // backward slow

// ── STEP TIMING
// This is how many milliseconds it takes to rotate 120°
// START with 600, then calibrate using + and - commands
int STEP_MS = 600;

// ── BIN TRACKING
int currentBin = 0;  // 0=Organic, 1=Paper, 2=Plastic
String binNames[] = { "ORGANIC", "PAPER", "PLASTIC" };

// ════════════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);
  disc.attach(DISC_PIN);

  // Stop servo on startup
  disc.write(STOP);
  delay(1000);

  Serial.println("══════════════════════════════════════");
  Serial.println("  ROTATING BIN TEST — IT 171");
  Serial.println("══════════════════════════════════════");
  Serial.println("COMMANDS:");
  Serial.println("  f → free spin forward");
  Serial.println("  b → free spin backward");
  Serial.println("  s → stop");
  Serial.println("  1 → Organic (home)");
  Serial.println("  2 → Paper (1 step)");
  Serial.println("  3 → Plastic (2 steps)");
  Serial.println("  h → return home");
  Serial.println("  + → increase step time +50ms");
  Serial.println("  - → decrease step time -50ms");
  Serial.println("  t → full auto test");
  Serial.println("  ? → print settings");
  Serial.println("══════════════════════════════════════");
  printStatus();
}

// ════════════════════════════════════════════════════════
// CORE MOVEMENT FUNCTIONS
// ════════════════════════════════════════════════════════

void stopDisc() {
  disc.write(STOP);
  delay(100);
}

// Rotate forward by one 120° step
void stepForward() {
  Serial.print("  → Rotating forward ");
  Serial.print(STEP_MS);
  Serial.println("ms...");
  disc.write(FWD_SLOW);
  delay(STEP_MS);
  stopDisc();
  delay(300); // let disc physically settle before continuing
  Serial.println("  ✓ Step complete");
}

// Rotate backward by one 120° step
void stepBackward() {
  Serial.print("  ← Rotating backward ");
  Serial.print(STEP_MS);
  Serial.println("ms...");
  disc.write(BWD_SLOW);
  delay(STEP_MS);
  stopDisc();
  delay(300);
  Serial.println("  ✓ Step complete");
}

// Go to a specific bin from current position
void goToBin(int targetBin) {
  if (targetBin == currentBin) {
    Serial.print("Already at ");
    Serial.println(binNames[targetBin]);
    return;
  }

  Serial.print("Moving from ");
  Serial.print(binNames[currentBin]);
  Serial.print(" → ");
  Serial.println(binNames[targetBin]);

  // Calculate steps needed
  // Forward path: 0→1→2
  // Backward path: 2→1→0 or 1→0
  int stepsForward  = (targetBin - currentBin + 3) % 3;
  int stepsBackward = (currentBin - targetBin + 3) % 3;

  // Take shortest path
  if (stepsForward <= stepsBackward) {
    for (int i = 0; i < stepsForward; i++) {
      stepForward();
    }
  } else {
    for (int i = 0; i < stepsBackward; i++) {
      stepBackward();
    }
  }

  currentBin = targetBin;
  Serial.print("✓ Now at: ");
  Serial.println(binNames[currentBin]);
}

void returnHome() {
  Serial.println("Returning to HOME (Organic)...");
  goToBin(0);
}

// ════════════════════════════════════════════════════════
// AUTO TEST
// ════════════════════════════════════════════════════════
void autoTest() {
  Serial.println("══ AUTO TEST STARTING ══");
  Serial.println("Watch each bin align under the drop point.");
  Serial.println("If alignment is off, adjust STEP_MS with + or -");
  Serial.println();

  Serial.println("[1/6] Going to ORGANIC (home)...");
  goToBin(0);
  Serial.println("      Is Organic bin under drop point? (y/n)");
  delay(2000);

  Serial.println("[2/6] Going to PAPER...");
  goToBin(1);
  Serial.println("      Is Paper bin under drop point? (y/n)");
  delay(2000);

  Serial.println("[3/6] Going to PLASTIC...");
  goToBin(2);
  Serial.println("      Is Plastic bin under drop point? (y/n)");
  delay(2000);

  Serial.println("[4/6] Back to PAPER...");
  goToBin(1);
  delay(1500);

  Serial.println("[5/6] Back to ORGANIC...");
  goToBin(0);
  delay(1500);

  Serial.println("[6/6] Final check — at PLASTIC...");
  goToBin(2);
  delay(1500);

  Serial.println("Returning home...");
  goToBin(0);

  Serial.println("══ AUTO TEST COMPLETE ══");
  Serial.print("Current STEP_MS = ");
  Serial.println(STEP_MS);
  Serial.println("If bins were not aligning, use + or - to adjust.");
}

// ════════════════════════════════════════════════════════
void printStatus() {
  Serial.println("──────────────────────────────────────");
  Serial.print("Current bin:  "); Serial.println(binNames[currentBin]);
  Serial.print("Step time:    "); Serial.print(STEP_MS); Serial.println("ms");
  Serial.print("Fwd speed:    "); Serial.println(FWD_SLOW);
  Serial.print("Bwd speed:    "); Serial.println(BWD_SLOW);
  Serial.println("──────────────────────────────────────");
}

// ════════════════════════════════════════════════════════
void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
      case 'f': case 'F':
        Serial.println("Free spin FORWARD (press s to stop)");
        disc.write(FWD_SLOW);
        break;

      case 'b': case 'B':
        Serial.println("Free spin BACKWARD (press s to stop)");
        disc.write(BWD_SLOW);
        break;

      case 's': case 'S':
        stopDisc();
        Serial.println("✓ Stopped");
        break;

      case '1':
        goToBin(0); // Organic
        break;

      case '2':
        goToBin(1); // Paper
        break;

      case '3':
        goToBin(2); // Plastic
        break;

      case 'h': case 'H':
        returnHome();
        break;

      case '+':
        STEP_MS += 50;
        Serial.print("Step time increased → ");
        Serial.print(STEP_MS);
        Serial.println("ms");
        break;

      case '-':
        if (STEP_MS > 100) STEP_MS -= 50;
        Serial.print("Step time decreased → ");
        Serial.print(STEP_MS);
        Serial.println("ms");
        break;

      case 't': case 'T':
        autoTest();
        break;

      case '?':
        printStatus();
        break;

      case '\n': case '\r':
        break;

      default:
        Serial.print("Unknown: '");
        Serial.print(cmd);
        Serial.println("' — type f/b/s/1/2/3/h/+/-/t/?");
    }
  }
}
