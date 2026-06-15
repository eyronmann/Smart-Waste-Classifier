PROJECT: Smart Waste Classifier
MEMBERS: Aron C. Mirano
	 Chaiden Mark U. Villanueva
	 Mart Vincent F. Rasos
	 Hans Rochie L. Galiza
	 Jerameel Cabading

HARDWARE USED:
- Arduino Uno R3
- ESP32-CAM (AI-Thinker) + MB board
- MG90S continuous rotation servo (disc)
- SG90 positional servo (trap door)
- HC-SR04 ultrasonic sensor
- 3x LEDs (green, yellow, blue) + 220Ω resistors
- Active buzzer
- Breadboard + jumper wires
- 5V 3A power adapter

LIBRARIES REQUIRED (install via Arduino IDE Library Manager):
- LiquidCrystal_I2C by Frank de Brabander
- SoftwareSerial (built-in)
- Servo (built-in)
- ESP32 board package by Espressif
- classified-capture-clone_inferencing
  (install manually via Sketch > Include Library > Add .ZIP)

HOW TO RUN:
1. Install all libraries above
2. Upload esp32cam_classifier_final.ino to ESP32-CAM
   - Board: AI Thinker ESP32-CAM
   - Hold IO0 before uploading, release when "Connecting..."
   - Press RST after upload
3. Upload main_project.ino to Arduino Uno
   - Board: Arduino Uno
   - Update STEP_MS value on line 16 for your disc calibration
4. Wire components per wiring diagram
5. Power on: plug 5V adapter to breadboard rail,
   plug Arduino to laptop USB
6. LCD shows SYSTEM READY when both boards are running
7. Place waste item on scan tray to classify

ML MODEL:
- Platform: Edge Impulse
- Classes: Metal, Paper, Plastic
- Input: 96x96 RGB image
- Accuracy: 90%
- Inference: on-device, no internet required