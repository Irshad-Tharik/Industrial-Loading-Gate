#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// ---------------- PIN CONNECTIONS ----------------
#define PIR_PIN 27
#define TRIG_PIN 5
#define ECHO_PIN 18
#define POT_PIN 34
#define SERVO_PIN 19
#define BUZZER_PIN 23
#define BUTTON_PIN 26

// ---------------- SYSTEM SETTINGS ----------------
#define CLOSED_ANGLE 0
#define OPEN_ANGLE 90
#define DETECT_CM 15       // Vehicle must be 15cm or closer to open
#define VEHICLE_CM 40      // Vehicle presence distance
#define SAFETY_CM 20       // Safety distance while gate is closing
#define RESET_CM 25        // Path must be above 25cm for reset

Servo gateServo;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Gate operating states
enum GateState {CLOSED, OPENING, OPEN, CLOSING, SAFETY};
GateState gate = CLOSED;

// System operating modes
enum Mode {AUTO_MODE, MANUAL_MODE, SAFETY_MODE};
Mode mode = AUTO_MODE;

// ---------------- VARIABLES ----------------
float distanceCM = 400;
int servoAngle = CLOSED_ANGLE;
int detectCount = 0;
unsigned long holdTime = 3000;
unsigned long openTimer = 0;
unsigned long servoTimer = 0;
unsigned long sensorTimer = 0;
unsigned long statusTimer = 0;
bool vehicleWasPresent = false;
bool closeTimerStarted = false;

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200); // Start Serial Monitor

  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, LOW);

  gateServo.attach(SERVO_PIN); // Connect servo
  gateServo.write(CLOSED_ANGLE); // Start with gate closed

  Wire.begin(21, 22); // SDA=21, SCL=22
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED NOT FOUND");
  }

  showOLED("GATE CLOSED", "AUTO MODE");
  Serial.println("SYSTEM READY");
}

// ---------------- MAIN LOOP ----------------
void loop() {
  readPotentiometer(); // Read adjustable 2-8 second delay
  readUltrasonic(); // Read vehicle distance
  readSerial(); // Read UART commands

  if (mode == AUTO_MODE) automaticMode();
  if (mode == SAFETY_MODE) safetyMode();

  updateServo(); // Move gate
  checkSafety(); // Check obstruction while closing
  printStatus(); // Show information in Serial Monitor
}

// ---------------- AUTOMATIC MODE ----------------
void automaticMode() {
  bool motion = digitalRead(PIR_PIN);

  // CLOSED: PIR detects motion and ultrasonic confirms vehicle within 15cm
  if (gate == CLOSED) {
    if (motion && distanceCM <= DETECT_CM) {
      detectCount++;
    } else {
      detectCount = 0;
    }

    // Two correct readings confirm a real vehicle
    if (detectCount >= 2) {
      detectCount = 0;
      vehicleWasPresent = false;
      closeTimerStarted = false;

      Serial.println("VEHICLE DETECTED");
      Serial.println("GATE OPENING");
      showOLED("VEHICLE DETECTED", "GATE OPENING");

      gate = OPENING;
    }
  }

  // OPEN: keep gate open while vehicle is within 40cm
  else if (gate == OPEN) {
    if (distanceCM < VEHICLE_CM) {
      vehicleWasPresent = true;
      closeTimerStarted = false;
      showOLED("GATE OPEN", "VEHICLE PRESENT");
    }

    // Vehicle has moved away
    else {
      if (vehicleWasPresent && !closeTimerStarted) {
        openTimer = millis();
        closeTimerStarted = true;

        Serial.println("VEHICLE LEFT");
        showOLED("VEHICLE LEFT", "WAITING");
      }

      // Start timer even if vehicle quickly passed through
      if (!vehicleWasPresent && !closeTimerStarted) {
        openTimer = millis();
        closeTimerStarted = true;
      }

      // Close after potentiometer selected delay
      if (closeTimerStarted && millis() - openTimer >= holdTime) {
        closeTimerStarted = false;
        vehicleWasPresent = false;

        Serial.println("GATE CLOSING");
        showOLED("ROAD CLEAR", "GATE CLOSING");

        gate = CLOSING;
      }
    }
  }
}

// ---------------- ULTRASONIC SENSOR ----------------
void readUltrasonic() {
  if (millis() - sensorTimer < 100) return; // Read every 100ms
  sensorTimer = millis();

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  // Use 400cm when there is no valid echo
  if (duration == 0) {
    distanceCM = 400;
  } else {
    distanceCM = duration * 0.034 / 2.0;
    if (distanceCM < 2) distanceCM = 400;
  }
}

// ---------------- POTENTIOMETER ----------------
void readPotentiometer() {
  int potValue = analogRead(POT_PIN); // Read ADC value 0-4095
  holdTime = map(potValue, 0, 4095, 2000, 8000); // Convert to 2-8 seconds
}

// ---------------- SERVO CONTROL ----------------
void updateServo() {
  if (millis() - servoTimer < 10) return; // Control servo speed
  servoTimer = millis();

  // Slowly open gate from 0 to 90 degrees
  if (gate == OPENING) {
    if (servoAngle < OPEN_ANGLE) {
      servoAngle++;
      gateServo.write(servoAngle);
    } else {
      gate = OPEN;
      vehicleWasPresent = false;
      closeTimerStarted = false;

      Serial.println("GATE OPEN");
      showOLED("GATE OPEN", "CHECKING VEHICLE");
    }
  }

  // Slowly close gate from 90 to 0 degrees
  else if (gate == CLOSING) {
    if (servoAngle > CLOSED_ANGLE) {
      servoAngle--;
      gateServo.write(servoAngle);
    } else {
      gate = CLOSED;
      Serial.println("GATE CLOSED");
      showOLED("GATE CLOSED", "AUTO MODE");
    }
  }
}

// ---------------- SAFETY CHECK ----------------
void checkSafety() {
  // Safety works only while gate is closing
  if (gate == CLOSING && distanceCM < SAFETY_CM) {
    gate = SAFETY;
    mode = SAFETY_MODE;

    digitalWrite(BUZZER_PIN, HIGH);

    Serial.println("SAFETY BLOCK");
    Serial.println("GATE STOPPED");

    showOLED("SAFETY BLOCK!", "REMOVE + RESET");
  }
}

// ---------------- SAFETY RESET ----------------
void safetyMode() {
  // Press button after obstruction is removed
  if (digitalRead(BUTTON_PIN) == LOW) {

    if (distanceCM > RESET_CM) {
      digitalWrite(BUZZER_PIN, LOW);

      Serial.println("SAFETY RESET");
      Serial.println("GATE REOPENING");

      showOLED("SAFETY RESET", "GATE REOPENING");

      mode = AUTO_MODE;
      gate = OPENING; // Reopen gate after safety stop
      vehicleWasPresent = false;
      closeTimerStarted = false;

      delay(300);
    }

    else {
      Serial.println("REMOVE OBSTRUCTION");
    }
  }
}

// ---------------- UART / SERIAL CONTROL ----------------
void readSerial() {
  if (!Serial.available()) return;

  char command = Serial.read();

  // A = Automatic mode
  if ((command == 'A' || command == 'a') && mode != SAFETY_MODE) {
    mode = AUTO_MODE;
    Serial.println("AUTO MODE");
  }

  // M = Manual mode
  else if ((command == 'M' || command == 'm') && mode != SAFETY_MODE) {
    mode = MANUAL_MODE;
    Serial.println("MANUAL MODE");
  }

  // O = Open gate in manual mode
  else if ((command == 'O' || command == 'o') && mode == MANUAL_MODE) {
    gate = OPENING;
    Serial.println("MANUAL OPEN");
  }

  // C = Close gate in manual mode
  else if ((command == 'C' || command == 'c') && mode == MANUAL_MODE) {
    gate = CLOSING;
    Serial.println("MANUAL CLOSE");
  }
}

// ---------------- OLED DISPLAY ----------------
void showOLED(String line1, String line2) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(5, 15);
  display.println(line1);

  display.setCursor(5, 35);
  display.println(line2);

  display.display();
}

// ---------------- SERIAL MONITOR ----------------
void printStatus() {
  if (millis() - statusTimer < 1000) return;
  statusTimer = millis();

  Serial.print("PIR: ");
  Serial.print(digitalRead(PIR_PIN) ? "MOTION" : "CLEAR");

  Serial.print(" | Distance: ");
  if (distanceCM >= 400) {
    Serial.print("NO ECHO");
  } else {
    Serial.print(distanceCM);
    Serial.print("cm");
  }

  Serial.print(" | Hold: ");
  Serial.print(holdTime / 1000.0);
  Serial.print("s");

  Serial.print(" | Mode: ");
  if (mode == AUTO_MODE) Serial.print("AUTO");
  else if (mode == MANUAL_MODE) Serial.print("MANUAL");
  else Serial.print("SAFETY");

  Serial.print(" | Gate: ");
  if (gate == CLOSED) Serial.println("CLOSED");
  else if (gate == OPENING) Serial.println("OPENING");
  else if (gate == OPEN) Serial.println("OPEN");
  else if (gate == CLOSING) Serial.println("CLOSING");
  else Serial.println("SAFETY");
}

