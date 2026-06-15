// ════════════════════════════════════════════════
// ROTATING BIN — FORWARD ONLY (updated)
// ════════════════════════════════════════════════
// Servo signal → D9
// Servo VCC    → Breadboard + (5V adapter)
// Servo GND    → Breadboard – (common GND)
// Arduino GND  → Breadboard – (IMPORTANT)
//
// Open Serial Monitor at 9600 baud
// Type 1, 2, or 3 to rotate to each bin
// Always rotates FORWARD only — never backward
// ════════════════════════════════════════════════

#include <Servo.h>
Servo disc;

// ── SETTINGS ─────────────────────────────────────
int stepTime = 750;   // calibrated forward step time
                      // use + and - to adjust

#define DISC_FWD   96  // forward speed
#define DISC_STOP  90  // stop value

int currentBin = 0;   // tracks current position
                      // 0=Organic 1=Paper 2=Plastic

// ════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);
  disc.attach(9);
  disc.write(DISC_STOP);
  delay(1000);

  Serial.println("════════════════════════════════");
  Serial.println("  ROTATING BIN — FORWARD ONLY");
  Serial.println("════════════════════════════════");
  Serial.println("1 = Organic");
  Serial.println("2 = Paper");
  Serial.println("3 = Plastic");
  Serial.println("+ = add 50ms to step time");
  Serial.println("- = remove 50ms from step time");
  Serial.println("? = show current settings");
  Serial.println("t = auto test all 3 bins");
  Serial.println("════════════════════════════════");
  Serial.println("Starting at Organic (home)");
  Serial.print("Step time: ");
  Serial.println(stepTime);
}

// ════════════════════════════════════════════════
// MOVE TO BIN — FORWARD ONLY
// ════════════════════════════════════════════════
void goToBin(int target) {
  String names[] = { "Organic", "Paper", "Plastic" };

  if (target == currentBin) {
    Serial.print("Already at ");
    Serial.println(names[target]);
    return;
  }

  // Always go forward — never backward
  // 3 bins × 120° each = full 360° circle
  int steps = (target - currentBin + 3) % 3;

  Serial.print("Moving to ");
  Serial.print(names[target]);
  Serial.print(" — ");
  Serial.print(steps);
  Serial.println(" step(s) forward");

  for (int i = 0; i < steps; i++) {
    disc.write(DISC_FWD);  // spin forward
    delay(stepTime);        // for this many ms
    disc.write(DISC_STOP);  // stop
    delay(500);             // let disc settle fully
  }

  currentBin = target;

  Serial.print("✓ Now at: ");
  Serial.println(names[currentBin]);
}

// ════════════════════════════════════════════════
// AUTO TEST — all 3 bins in sequence
// ════════════════════════════════════════════════
void autoTest() {
  Serial.println("════ AUTO TEST START ════");
  Serial.println("Watch each bin align under drop point");

  goToBin(0);
  Serial.println("Is ORGANIC aligned? Check now...");
  delay(2000);

  goToBin(1);
  Serial.println("Is PAPER aligned? Check now...");
  delay(2000);

  goToBin(2);
  Serial.println("Is PLASTIC aligned? Check now...");
  delay(2000);

  goToBin(0);
  Serial.println("Back to ORGANIC — aligned?");
  delay(2000);

  goToBin(2);
  Serial.println("PLASTIC again — aligned?");
  delay(2000);

  goToBin(1);
  Serial.println("PAPER again — aligned?");
  delay(2000);

  goToBin(0);
  Serial.println("════ AUTO TEST DONE ════");
  Serial.print("Step time used: ");
  Serial.println(stepTime);
  Serial.println("If any bin was off, adjust with + or -");
}

// ════════════════════════════════════════════════
void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
      case '1':
        goToBin(0); // Organic
        break;

      case '2':
        goToBin(1); // Paper
        break;

      case '3':
        goToBin(2); // Plastic
        break;

      case '+':
        stepTime += 50;
        Serial.print("Step time increased → ");
        Serial.print(stepTime);
        Serial.println("ms");
        break;

      case '-':
        if (stepTime > 100) stepTime -= 50;
        Serial.print("Step time decreased → ");
        Serial.print(stepTime);
        Serial.println("ms");
        break;

      case 't': case 'T':
        autoTest();
        break;

      case '?':
        Serial.println("──────────────────────");
        Serial.print("Current bin:  ");
        String names[] = { "Organic", "Paper", "Plastic" };
        Serial.println(names[currentBin]);
        Serial.print("Step time:    ");
        Serial.print(stepTime);
        Serial.println("ms");
        Serial.print("Forward speed: ");
        Serial.println(DISC_FWD);
        Serial.print("Stop value:    ");
        Serial.println(DISC_STOP);
        Serial.println("──────────────────────");
        break;

      case '\n': case '\r':
        break;

      default:
        Serial.print("Unknown: ");
        Serial.print(cmd);
        Serial.println(" — use 1/2/3/+/-/t/?");
    }
  }
}
