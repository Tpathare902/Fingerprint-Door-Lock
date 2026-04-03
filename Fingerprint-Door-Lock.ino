/*************************************************************
  Fingerprint Door Lock System
  Hardware:
    - Arduino Nano
    - Adafruit Optical Fingerprint Sensor (TTL Serial)
    - JHD162A 16x2 LCD (4-bit parallel)
    - SG90 / MG995 Servo Motor
    - Push Button (D8 → GND, uses internal pull-up)

  LCD Wiring (JHD162A → Arduino Nano):
    Pin 1  VSS  → GND
    Pin 2  VDD  → 5V
    Pin 3  V0   → 10K pot wiper
    Pin 4  RS   → D12
    Pin 5  RW   → GND
    Pin 6  E    → D11
    Pin 7-10     → Not connected (4-bit mode)
    Pin 11 D4   → D5
    Pin 12 D5   → D4
    Pin 13 D6   → D3
    Pin 14 D7   → D2
    Pin 15 A    → 5V (via 220Ω resistor)
    Pin 16 K    → GND

  Fingerprint Sensor Wiring:
    RED   (VCC) → 3.3V or 5V
    BLACK (GND) → GND
    GREEN (TX)  → D6  (Arduino RX from sensor)
    WHITE (RX)  → D7  (Arduino TX to sensor)

  Servo Wiring:
    RED    → 5V (external supply recommended)
    BROWN  → GND (shared with Arduino GND)
    ORANGE → D9 (signal)

  Button Wiring:
    One leg → D8
    Other leg → GND
    (Internal pull-up used — no resistor needed)
*************************************************************/

#include <LiquidCrystal.h>
#include <Adafruit_Fingerprint.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// ── LCD ──────────────────────────────────────────────────
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// ── Fingerprint sensor on D6 (RX) and D7 (TX) ───────────
SoftwareSerial fingerSerial(6, 7);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

// ── Servo ────────────────────────────────────────────────
Servo lockServo;
const int SERVO_PIN    = 9;
const int SERVO_LOCKED = 0;
const int SERVO_OPEN   = 90;
const int OPEN_DELAY   = 5000;

// ── Button ───────────────────────────────────────────────
const int BUTTON_PIN = 8;
bool enrollMode      = false;
bool lastButtonState = HIGH;  // pull-up: idle = HIGH

// ── Setup ────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // LCD init
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  DOOR  LOCK  ");
  lcd.setCursor(0, 1);
  lcd.print(" Initializing..");
  delay(1500);

  // Servo — lock on boot
  lockServo.attach(SERVO_PIN);
  lockServo.write(SERVO_LOCKED);
  delay(300);

  // Fingerprint sensor init
  finger.begin(57600);
  delay(5);

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor OK!");
    lcd.setCursor(0, 1);
    lcd.print("Loading...");
    delay(1000);
  } else {
    Serial.println("Fingerprint sensor NOT found!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SENSOR ERROR!");
    lcd.setCursor(0, 1);
    lcd.print("Check wiring..");
    while (1) { delay(1); }
  }

  finger.getTemplateCount();
  Serial.print("Templates stored: ");
  Serial.println(finger.templateCount);

  if (finger.templateCount == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No fingerprints!");
    lcd.setCursor(0, 1);
    lcd.print("Press btn 2 add");
    delay(3000);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ready!          ");
    lcd.setCursor(0, 1);
    lcd.print(finger.templateCount);
    lcd.print(" user(s) stored");
    delay(2000);
  }

  showIdle();
}

// ── Main Loop ────────────────────────────────────────────
void loop() {
  checkButton();  // Always check for button press first

  if (enrollMode) {
    runEnrollment();
    // After one enrollment attempt (success or fail), stay in
    // enroll mode until button is pressed again
  } else {
    // Continuous verification
    int id = getFingerprintID();

    if (id > 0) {
      Serial.print("ACCESS GRANTED — ID #");
      Serial.println(id);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ACCESS GRANTED!");
      lcd.setCursor(0, 1);
      lcd.print("User ID: ");
      lcd.print(id);

      lockServo.write(SERVO_OPEN);
      delay(OPEN_DELAY);
      lockServo.write(SERVO_LOCKED);
      delay(500);

      showIdle();

    } else if (id == -2) {
      Serial.println("ACCESS DENIED");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ACCESS DENIED! ");
      lcd.setCursor(0, 1);
      lcd.print("Unknown finger ");
      delay(2000);

      showIdle();
    }
    // id == -1 → no finger yet, keep polling silently
  }

  delay(100);
}

// ── Button Check (non-blocking, debounced) ───────────────
void checkButton() {
  bool currentState = digitalRead(BUTTON_PIN);

  // Detect falling edge (HIGH → LOW = button pressed)
  if (lastButtonState == HIGH && currentState == LOW) {
    delay(50);  // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      enrollMode = !enrollMode;

      if (enrollMode) {
        Serial.println("Enroll mode ON");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("-- ENROLL MODE -");
        lcd.setCursor(0, 1);
        lcd.print("Starting...     ");
        delay(1000);
      } else {
        Serial.println("Enroll mode OFF — back to verify");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enroll stopped  ");
        lcd.setCursor(0, 1);
        lcd.print("Verifying...    ");
        delay(1500);
        showIdle();
      }
    }
  }

  lastButtonState = currentState;
}

// ── Enrollment Routine ───────────────────────────────────
// Automatically uses the next available slot ID.
// Called repeatedly while enrollMode == true.
void runEnrollment() {
  // Find next free ID
  finger.getTemplateCount();
  uint8_t newID = finger.templateCount + 1;

  if (newID > 127) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Memory full!    ");
    lcd.setCursor(0, 1);
    lcd.print("Max 127 users   ");
    delay(3000);
    return;
  }

  Serial.print("Enrolling ID #");
  Serial.println(newID);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enrolling ID #");
  lcd.print(newID);
  lcd.setCursor(0, 1);
  lcd.print("Place finger... ");

  // ── Scan 1 ───────────────────────────────────────────
  uint8_t p = -1;
  while (p != FINGERPRINT_OK) {
    checkButton();             // Allow cancel mid-enroll
    if (!enrollMode) return;   // Button pressed — abort

    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) continue;
    if (p == FINGERPRINT_OK) break;

    lcd.setCursor(0, 1);
    lcd.print("Imaging error   ");
    delay(500);
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Bad image!      ");
    lcd.setCursor(0, 1);
    lcd.print("Try again...    ");
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Remove finger   ");
  delay(2000);

  // Wait until finger is lifted
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
    checkButton();
    if (!enrollMode) return;
  }

  // ── Scan 2 ───────────────────────────────────────────
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place SAME      ");
  lcd.setCursor(0, 1);
  lcd.print("finger again... ");

  p = -1;
  while (p != FINGERPRINT_OK) {
    checkButton();
    if (!enrollMode) return;

    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) continue;
    if (p == FINGERPRINT_OK) break;
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Bad image!      ");
    lcd.setCursor(0, 1);
    lcd.print("Try again...    ");
    delay(2000);
    return;
  }

  // ── Create model & store ──────────────────────────────
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Prints differ!  ");
    lcd.setCursor(0, 1);
    lcd.print("Try again...    ");
    delay(2000);
    return;
  }

  p = finger.storeModel(newID);
  if (p == FINGERPRINT_OK) {
    Serial.print("Stored ID #");
    Serial.println(newID);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enrolled! ID #");
    lcd.print(newID);
    lcd.setCursor(0, 1);
    lcd.print("Press btn=done  ");
    delay(3000);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Store failed!   ");
    lcd.setCursor(0, 1);
    lcd.print("Try again...    ");
    delay(2000);
  }
}

// ── Fingerprint Scan (Verify) ────────────────────────────
int getFingerprintID() {
  uint8_t p;

  p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER) return -1;
  if (p != FINGERPRINT_OK)       return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Bad image!     ");
    lcd.setCursor(0, 1);
    lcd.print("Try again...   ");
    delay(1500);
    showIdle();
    return -1;
  }

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK)       return finger.fingerID;
  if (p == FINGERPRINT_NOTFOUND) return -2;

  return -1;
}

// ── Idle Screen ──────────────────────────────────────────
void showIdle() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place finger    ");
  lcd.setCursor(0, 1);
  lcd.print("on sensor...    ");
}
