#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// -------- BLYNK CONFIGURATION --------
// ⚠️ REPLACE THESE WITH YOUR OWN DETAILS BEFORE UPLOADING TO BOARD ⚠️
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID_HERE"
#define BLYNK_TEMPLATE_NAME "Smart Parking"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_TOKEN_HERE"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// ⚠️ REPLACE THESE WITH YOUR WIFI DETAILS ⚠️
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// Non-blocking connect control
unsigned long lastConnTry = 0;
const unsigned long WIFI_RETRY_MS  = 5000;
const unsigned long BLYNK_RETRY_MS = 5000;

// Pace Blynk writes
unsigned long lastBlynkWrite = 0;
const unsigned long BLYNK_UPDATE_MS = 500;

// -------- I2C LCD --------
#define SDA_PIN 21
#define SCL_PIN 22

// -------- IR Sensors for slots (HIGH = Free, LOW = Full) --------
#define SLOT1 14
#define SLOT2 27
#define SLOT3 26
#define SLOT4 25

// -------- Exit IR sensor (active LOW) --------
#define IR_EXIT 35

// -------- Ultrasonic Sensor Pins (Entry) --------
#define TRIG_PIN 33
#define ECHO_PIN 34

// -------- Servo --------
#define SERVO_GATE 13

// -------- LEDs --------
#define GREEN_LED 12
#define RED_LED   4

// -------- LCD & Servo Objects --------
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo gateServo;

// -------- Vars --------
long duration;
float distance_cm;

// Parking stats (count via slot transitions only)
const int TOTAL_SLOTS = 4;
volatile unsigned long carsIn  = 0;
volatile unsigned long carsOut = 0;

// Slot debounce/state tracking
const unsigned long DEBOUNCE_MS = 120;
const uint8_t slotPins[TOTAL_SLOTS] = {SLOT1, SLOT2, SLOT3, SLOT4};
// stableState: HIGH = Free, LOW = Full
bool  stableState[TOTAL_SLOTS];
unsigned long lastChangeAt[TOTAL_SLOTS];

// Entry detection (ultrasonic) — only for gate comfort (no counting)
const float ENTRY_THRESHOLD_CM = 10.0;

// Display paging
unsigned long lastPageSwitch = 0;
const unsigned long PAGE_PERIOD_MS = 5000;
uint8_t pageIndex = 0; // 0 = slot page, 1 = stats page

// ---------- helpers ----------
void measureDistance();
void showSlotsPage(bool s1, bool s2, bool s3, bool s4);
void showStatsPage(int freeSlots);
void openGate();

void setup() {
  Serial.begin(115200);

  // ----- WiFi + Blynk (NON-BLOCKING) -----
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);         // start WiFi (non-blocking)
  Blynk.config(auth);             // set token (no WiFi needed here)

  // LCD init
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.begin(16, 2);
  lcd.backlight();

  // IR slot sensors
  pinMode(SLOT1, INPUT);
  pinMode(SLOT2, INPUT);
  pinMode(SLOT3, INPUT);
  pinMode(SLOT4, INPUT);

  // Initialize slot stable states
  for (int i = 0; i < TOTAL_SLOTS; i++) {
    stableState[i] = digitalRead(slotPins[i]); // HIGH=Free, LOW=Full
    lastChangeAt[i] = millis();
  }

  // Exit sensor
  pinMode(IR_EXIT, INPUT); 

  // Ultrasonic pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // LEDs
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  // Servo
  gateServo.setPeriodHertz(50);
  gateServo.attach(SERVO_GATE, 500, 2400);
  gateServo.write(0);  // Closed

  Serial.println("Setup done. Trying WiFi/Blynk in background...");
}

void loop() {
  // ---- Keep app logic ALWAYS running ----
  measureDistance();

  unsigned long now = millis();
  int freeSlots = 0;

  // Read & debounce slots; update IN/OUT
  for (int i = 0; i < TOTAL_SLOTS; i++) {
    bool current = digitalRead(slotPins[i]); // HIGH=Free, LOW=Full

    if (current != stableState[i]) {
      if (now - lastChangeAt[i] >= DEBOUNCE_MS) {
        bool prev = stableState[i];
        stableState[i] = current;
        lastChangeAt[i] = now;

        if (prev == HIGH && current == LOW)      carsIn++;
        else if (prev == LOW && current == HIGH) carsOut++;
      }
    } else {
      lastChangeAt[i] = now - DEBOUNCE_MS;
    }

    if (stableState[i] == HIGH) freeSlots++;
  }

  bool allFull = (freeSlots == 0);
  digitalWrite(RED_LED, allFull ? HIGH : LOW);

  // Gate comfort
  if (distance_cm > 0 && distance_cm < ENTRY_THRESHOLD_CM && !allFull) {
    openGate();
  }
  if (digitalRead(IR_EXIT) == LOW) {
    openGate();
  }

  // Display paging
  if (now - lastPageSwitch >= PAGE_PERIOD_MS) {
    pageIndex ^= 1;
    lastPageSwitch = now;
  }

  bool s1 = stableState[0];
  bool s2 = stableState[1];
  bool s3 = stableState[2];
  bool s4 = stableState[3];

  if (pageIndex == 0) showSlotsPage(s1, s2, s3, s4);
  else                showStatsPage(freeSlots);

  // ---- Non-blocking WiFi/Blynk maintenance ----
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastConnTry >= WIFI_RETRY_MS) {
      WiFi.disconnect(true);
      WiFi.begin(ssid, pass);
      lastConnTry = now;
    }
  } else {
    if (!Blynk.connected() && now - lastConnTry >= BLYNK_RETRY_MS) {
      Blynk.connect(1000); 
      lastConnTry = now;
    }
  }

  Blynk.run();

  if (Blynk.connected() && (now - lastBlynkWrite >= BLYNK_UPDATE_MS)) {
    Blynk.virtualWrite(V0, (int)carsIn);
    Blynk.virtualWrite(V1, (int)carsOut);
    Blynk.virtualWrite(V2, (int)freeSlots);
    Blynk.virtualWrite(V3, s1 ? "Free" : "Full");
    Blynk.virtualWrite(V4, s2 ? "Free" : "Full");
    Blynk.virtualWrite(V5, s3 ? "Free" : "Full");
    Blynk.virtualWrite(V6, s4 ? "Free" : "Full");
    lastBlynkWrite = now;
  }
  delay(50);
}

// ----------- Functions -----------
void measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH, 30000); 
  if (duration == 0) distance_cm = 1000;
  else distance_cm = duration * 0.034f / 2.0f;
}

void showSlotsPage(bool s1, bool s2, bool s3, bool s4) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("S1:"); lcd.print(s1 ? "Free" : "Full ");
  lcd.setCursor(9, 0); lcd.print("S2:"); lcd.print(s2 ? "Free" : "Full ");
  lcd.setCursor(0, 1); lcd.print("S3:"); lcd.print(s3 ? "Free" : "Full ");
  lcd.setCursor(9, 1); lcd.print("S4:"); lcd.print(s4 ? "Free" : "Full ");
}

void showStatsPage(int freeSlots) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("In:"); lcd.print((unsigned long)carsIn);
  lcd.print(" Out:"); lcd.print((unsigned long)carsOut);
  lcd.setCursor(0, 1); lcd.print("Free:"); lcd.print(freeSlots);
}

void openGate() {
  digitalWrite(GREEN_LED, HIGH);
  gateServo.write(90);  
  delay(2400);          
  gateServo.write(0);   
  digitalWrite(GREEN_LED, LOW);
}
