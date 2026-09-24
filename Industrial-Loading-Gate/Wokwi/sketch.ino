const int PIR_PIN = 27;

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;

const int POT_PIN = 34;

#include <ESP32Servo.h>
const int SERVO_PIN = 19;
Servo gateServo;

const int BUZZER_PIN = 23;

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);


void setup() {

  Serial.begin(115200);

  // PIR
  pinMode(PIR_PIN, INPUT);

  // Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  // gate servo 
  gateServo.attach(SERVO_PIN);
  pinMode(BUZZER_PIN, OUTPUT);

  //Oled display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);

  display.println("Loading Gate");

  display.display();
}



void loop() {

  // -------- PIR --------

  int motion = digitalRead(PIR_PIN);

  if (motion == HIGH) {
    Serial.println("Motion Detected!");
  } else {
    Serial.println("No Motion");
  }


  // -------- ULTRASONIC --------

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);

  float distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");


  delay(500);

  // potentiometer
  int potValue = analogRead(POT_PIN);

  Serial.print("Potentiometer: ");
  Serial.println(potValue);

  // Servo
  gateServo.write(0);
  delay(2000);

  gateServo.write(90);
  delay(2000);

  // Buzzer
  digitalWrite(BUZZER_PIN, HIGH);
delay(500);

digitalWrite(BUZZER_PIN, LOW);
delay(500);

}