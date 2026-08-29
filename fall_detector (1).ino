#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <SoftwareSerial.h>

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
sensors_event_t event;

#define SIM800_TX D5
#define SIM800_RX D6
SoftwareSerial sim800(SIM800_TX, SIM800_RX);

#define BUZZER_PIN D8
#define BUTTON_PIN D7

#define HARDCODED_LOCATION "http://maps.google.com/maps?q=18.492205,74.025441"

float previousRoll  = 0.0;
float previousPitch = 0.0;
bool  buttonPressed = false;
unsigned long lastFallTime = 0;

// ─────────────────────────────────────────────
// Component status flags — set during setup
// ─────────────────────────────────────────────
bool accel_ok  = false;
bool buzzer_ok = false; // assumed OK if pin responds
bool sim_ok    = false;
bool simcard_ok = false;
bool signal_ok = false;

void printDivider() { Serial.println("------------------------------------"); }

// ─────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────

void setup() {
  Wire.begin(0, 2); // SDA = GPIO0 (D3), SCL = GPIO2 (D4)
  Serial.begin(9600);
  delay(500);
  printDivider();
  Serial.println("[BOOT] Fall Detection System — full component check");
  printDivider();

  // ── 1. ADXL345 ──────────────────────────────
  Serial.println("[1/4] Checking ADXL345 accelerometer...");
  if (accel.begin()) {
    accel.setRange(ADXL345_RANGE_16_G);
    accel_ok = true;
    Serial.println("      PASS — ADXL345 found and ready.");
  } else {
    Serial.println("      FAIL — ADXL345 not responding over I2C.");
    Serial.println("      FIXES TO TRY:");
    Serial.println("        1. Connect CS pin -> 3.3V (forces I2C mode)");
    Serial.println("        2. Check SDA -> D2, SCL -> D1");
    Serial.println("        3. Check VCC -> 3V3, GND -> GND");
    Serial.println("        4. Try a different I2C address: accel = Adafruit_ADXL345_Unified(12345, 0x53)");
  }

  // ── 2. BUZZER ───────────────────────────────
  Serial.println("[2/4] Checking buzzer...");
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_PIN, LOW);
  buzzer_ok = true;
  Serial.println("      DONE — short beep fired on D8.");
  Serial.println("      Did you hear a beep? If not, check + -> D8, - -> GND.");
  Serial.println("      Also confirm it is an ACTIVE buzzer, not passive.");

  // ── 3. BUTTON ───────────────────────────────
  Serial.println("[3/4] Checking button...");
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  int btnState = digitalRead(BUTTON_PIN);
  if (btnState == HIGH) {
    Serial.println("      PASS — button reads HIGH (not pressed). Wiring looks correct.");
  } else {
    Serial.println("      WARN — button reads LOW at startup (not pressed).");
    Serial.println("      This usually means the pin is floating or wired incorrectly.");
    Serial.println("      Check: one leg -> D7, other leg -> GND.");
  }

  // ── 4. SIM800C ──────────────────────────────
  Serial.println("[4/4] Checking SIM800C GSM module...");
  sim800.begin(9600);
  delay(1000);

  Serial.println("      Sending AT ping...");
  sim800.println("AT");
  delay(600);
  String r1 = "";
  while (sim800.available()) r1 += (char)sim800.read();
  r1.trim();

  if (r1.indexOf("OK") >= 0) {
    sim_ok = true;
    Serial.println("      PASS — SIM800C responded OK.");
  } else {
    Serial.println("      FAIL — SIM800C did not respond to AT.");
    Serial.println("      FIXES TO TRY:");
    Serial.println("        1. Check power — needs 4.2V, up to 2A (NOT from NodeMCU 3.3V)");
    Serial.println("        2. Check TX/RX: SIM TX->D6, SIM RX->D5 (cross them!)");
    Serial.println("        3. All GNDs must share a common ground with NodeMCU");
    Serial.println("        4. Some modules need baud 115200 — try changing sim800.begin() rate");
  }

  if (sim_ok) {
    Serial.println("      Checking SIM card...");
    sim800.println("AT+CIMI");
    delay(600);
    String r2 = "";
    while (sim800.available()) r2 += (char)sim800.read();
    r2.trim();
    if (r2.indexOf("ERROR") >= 0 || r2.length() == 0) {
      Serial.println("      FAIL — SIM card not detected.");
      Serial.println("        Check SIM is inserted correctly, gold contacts facing down.");
      Serial.println("        This module does NOT support Jio (no 2G). Use Airtel or Vi.");
    } else {
      simcard_ok = true;
      Serial.println("      PASS — SIM card detected.");
      sim800.println("AT+CLCC=1");  // enable call status
      delay(300);
    }

    Serial.println("      Checking network signal...");
    sim800.println("AT+CSQ");
    delay(600);
    String r3 = "";
    while (sim800.available()) r3 += (char)sim800.read();
    r3.trim();
    Serial.println("      Signal response: " + r3);
    if (r3.indexOf("99") >= 0) {
      Serial.println("      WARN — CSQ=99 means no signal. Check SIM, antenna, and 2G coverage.");
    } else if (r3.indexOf("+CSQ") >= 0) {
      signal_ok = true;
      Serial.println("      PASS — Network signal detected.");
      Serial.println("      (CSQ: 0-9=poor, 10-14=ok, 15-19=good, 20+=excellent)");
    }
  }

  // ── FULL REPORT ─────────────────────────────
  printDivider();
  Serial.println("[REPORT] Component status summary:");
  Serial.println("  Accelerometer (ADXL345) : " + String(accel_ok   ? "OK" : "FAIL"));
  Serial.println("  Buzzer                  : " + String(buzzer_ok  ? "OK (verify by ear)" : "UNKNOWN"));
  Serial.println("  Button                  : " + String(digitalRead(BUTTON_PIN) == HIGH ? "OK" : "CHECK WIRING"));
  Serial.println("  SIM800C module          : " + String(sim_ok     ? "OK" : "FAIL"));
  Serial.println("  SIM card                : " + String(simcard_ok ? "OK" : (sim_ok ? "FAIL" : "SKIPPED")));
  Serial.println("  Network signal          : " + String(signal_ok  ? "OK" : (sim_ok ? "FAIL" : "SKIPPED")));
  printDivider();

  if (!accel_ok) {
    Serial.println("[WARN] Accelerometer failed — fall detection is DISABLED.");
    Serial.println("       Fix the accelerometer and reset before using the device.");
  }
  if (!sim_ok || !simcard_ok || !signal_ok) {
    Serial.println("[WARN] GSM issues detected — SMS and calling will NOT work.");
  }
  if (accel_ok && sim_ok && simcard_ok && signal_ok) {
    Serial.println("[READY] All components OK. System is fully operational.");
  }
  printDivider();
}

// ─────────────────────────────────────────────
// MAIN LOOP
// ─────────────────────────────────────────────
void loop() {
  // Skip fall detection entirely if accelerometer failed
  if (!accel_ok) {
    Serial.println("[ERROR] Accelerometer not available. Cannot monitor for falls. Reset after fixing.");
    delay(5000);
    return;
  }

  accel.getEvent(&event);
  float ax = event.acceleration.x;
  float ay = event.acceleration.y;
  float az = event.acceleration.z;

  if (ax == 0.0 && ay == 0.0 && az == 0.0) {
    Serial.println("[WARN]  Accelerometer returning all zeros — check wiring.");
  }

  float roll  = atan2(-ay, az) * 180.0 / PI;
  float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
  float deltaRoll  = abs(roll  - previousRoll);
  float deltaPitch = abs(pitch - previousPitch);
  previousRoll  = roll;
  previousPitch = pitch;

  float thresholdRoll  = 20.0;
  float thresholdPitch = 20.0;

  Serial.print("[SENSOR] Roll: ");   Serial.print(roll, 1);
  Serial.print("°  Pitch: ");        Serial.print(pitch, 1);
  Serial.print("°  |  dRoll: ");     Serial.print(deltaRoll, 1);
  Serial.print("°  dPitch: ");       Serial.print(deltaPitch, 1);
  Serial.println("°");

  unsigned long timeSinceLast = millis() - lastFallTime;
  if (lastFallTime > 0 && timeSinceLast < 5000) {
    Serial.print("[INFO]  Cooldown active — ");
    Serial.print((5000 - timeSinceLast) / 1000.0, 1);
    Serial.println("s remaining.");
  }

  if ((deltaRoll > thresholdRoll || deltaPitch > thresholdPitch)
      && (millis() - lastFallTime > 5000)) {

    printDivider();
    Serial.println("[FALL]  *** FALL DETECTED ***");
    Serial.print("[FALL]  dRoll="); Serial.print(deltaRoll, 1);
    Serial.print("°  dPitch="); Serial.print(deltaPitch, 1); Serial.println("°");

    // Warn if GSM won't work before even trying
    if (!sim_ok || !simcard_ok || !signal_ok) {
      Serial.println("[WARN]  GSM was not OK at startup — alert may not send!");
    }

    printDivider();
    triggerAlarm();

    Serial.println("[WAIT]  Waiting up to 10s for button press to cancel...");
    if (!waitForButton()) {
      Serial.println("[ALERT] No button press — sending alert!");
      sendAlertWithHardcodedLocation();
      delay(5000);
      makeCall();
    } else {
      printDivider();
      Serial.println("[CANCEL] Button pressed — alert cancelled.");
      printDivider();
    }

    lastFallTime  = millis();
    buttonPressed = false;
  }

  delay(100);
}

// ─────────────────────────────────────────────
// FUNCTIONS
// ─────────────────────────────────────────────

void triggerAlarm() {
  Serial.println("[BUZZER] Buzzer ON for 1 second...");
  digitalWrite(BUZZER_PIN, HIGH);
  delay(1000);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("[BUZZER] Buzzer OFF.");
}

bool waitForButton() {
  for (int i = 0; i < 100; i++) {
    delay(100);
    if (i % 10 == 0) {
      Serial.print("[WAIT]  "); Serial.print(10 - (i / 10)); Serial.println("s remaining...");
    }
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("[BUTTON] Button press detected!");
      buttonPressed = true;
      return true;
    }
  }
  Serial.println("[WAIT]  Timed out. No button press.");
  return false;
}

void sendAlertWithHardcodedLocation() {
  if (!buttonPressed) {
    printDivider();
    Serial.println("[SMS]   Sending SMS...");
    sim800.println("AT+CMGF=1");
    delay(300);
    String r1 = ""; while (sim800.available()) r1 += (char)sim800.read(); r1.trim();
    Serial.println("[SMS]   Text mode response: " + (r1.length() ? r1 : "(none)"));

    sim800.println("AT+CMGS=\"+919139551066\"");
    delay(300);

    sim800.print("FALL DETECTED! Check the patient, Else he might DIE!!!!!! You only have 2 min to reach, go immediately!! Location: MIT LONI ");
    sim800.print(HARDCODED_LOCATION);
    sim800.write(26);
    delay(10000);

    String r3 = ""; while (sim800.available()) r3 += (char)sim800.read(); r3.trim();
    if (r3.indexOf("+CMGS") >= 0) {
      Serial.println("[SMS]   SUCCESS. Confirmation: " + r3);
    } else if (r3.indexOf("ERROR") >= 0) {
      Serial.println("[ERROR] SMS FAILED: " + r3);
      Serial.println("        Check SIM credit, signal, and number format (+91...).");
    } else {
      Serial.println("[SMS]   Response: " + (r3.length() ? r3 : "(no confirmation)"));
    }
    printDivider();
  }
  clearSIM800Buffer();
}

void makeCall() {
  if (!buttonPressed) {
    clearSIM800Buffer();
    printDivider();
    Serial.println("[CALL]  Calling +919139551066...");
    sim800.println("ATD+919139551066;");
    delay(1000);

    String response = "";
    while (sim800.available()) {
      response += (char)sim800.read();
    }
    response.trim();

    Serial.println("[CALL] Dial response: " + response);
    

    // Wait up to 30s for the call to connect or fail
    bool callConnected = false;
    unsigned long callStart = millis();

    while (millis() - callStart < 30000) {
      if (sim800.available()) {
        String status = "";
        while (sim800.available()) status += (char)sim800.read();
        status.trim();
        Serial.println("[CALL]  Status: " + status);

        if (status.indexOf("NO CARRIER") >= 0 ||
            status.indexOf("BUSY")       >= 0 ||
            status.indexOf("NO ANSWER") >= 0) {
          Serial.println("[CALL]  Call did not connect: " + status);
          return;
        }
      }
      delay(500);
    }

    // Call was active for 30s — now hang up
    Serial.println("[CALL]  Hanging up after 30s...");
    sim800.println("ATH");
    delay(300);
    String hangupResp = "";
    while (sim800.available()) hangupResp += (char)sim800.read();
    Serial.println("[CALL]  Hangup response: " + hangupResp);
    Serial.println("[CALL]  Call ended.");
    printDivider();
  }
}


void clearSIM800Buffer() {
  while (sim800.available()) {
    sim800.read();
  }
}