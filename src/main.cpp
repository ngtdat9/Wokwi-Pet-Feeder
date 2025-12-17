// === Pet Feeder v2.0 (ESP32) ===
// Pins: HX711 DT=4, SCK=5 | Servo=18 | I2C SDA=21, SCL=22 | Buttons: 12/13/14/15 (to GND)
// Power: ESP32+HX711 @3.3V; RTC+LCD+Servo @5V (common GND)

#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <HX711.h>
#include <math.h>  // for fabs()

//web UI
#include <WiFi.h>
#include <WebServer.h>
#include "web_ui.h"


// ---- WiFi & Web ----
const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
WebServer server(80);

// ---- Pins ----
#define HX711_DT_PIN   4
#define HX711_SCK_PIN  5
#define SERVO_PIN      18
#define BUTTON_DISPLAY 12
#define BUTTON_SETTING 13
#define BUTTON_UP      14
#define BUTTON_DOWN    15


// ---- HW objects ----
RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(0x27, 20, 4);   // If blank, try 0x3F
Servo feedServo;
HX711 scale;

// ---- State ----
bool showSlots = false;
int  currentSlot = 0;
float currentWeight = 0.0f;
bool feedingActive = false;
bool feederOpen = false;
int  servoCloseAngle = 0;
int  servoOpenAngle  = 180;
float calibration_factor = -7050;     // adjust for your load cell
int  activeFeedingSlot = -1;
bool rtc_ok = false;

// ---- SIMULATION FLAG (Option A) ----
// Set to 1 in Wokwi, set to 0 on real hardware.
#ifndef SIM_FAKE_WEIGHT
#define SIM_FAKE_WEIGHT 1
#endif

// Simulated bowl weight (for Wokwi)
static float simWeight = 0.0f;

// ---- Safety & stuck detection ----
unsigned long feedingStartMs = 0;
const unsigned long FEED_TIMEOUT_MS = 15000;      // 15s safety timeout
float lastWeightDuringFeed = 0;
unsigned long lastWeightChangeMs = 0;
const unsigned long STUCK_WINDOW_MS = 4000;       // 4s with no increase -> consider stuck
const float MIN_INCREASE_G = 2.0;                 // noise floor for "increase"

// ---- Slots ----
struct FeedingSlot {
  bool  active;
  int   hour;
  int   minute;
  float weight;  // grams
};

FeedingSlot slots[3] = {
  {false,  8, 0, 0},
  {false, 12, 0, 0},
  {false, 18, 0, 0}
};

// ---- Feed history log ----
struct FeedLogEntry {
  bool  used;
  bool  manual;       // true = manual, false = scheduled slot
  int   slotIndex;    // -1 for manual
  int   hour;
  int   minute;
  float target;       // target weight (g)
  float finalWeight;  // final measured weight (g)
};

const int MAX_FEED_LOGS = 10;
FeedLogEntry feedLog[MAX_FEED_LOGS];
int feedLogCount = 0;

void addFeedLog(bool manual, int slotIndex, float target, float finalWeight);


// ---- Setting UI ----
enum SettingState {
  NOT_SETTING,
  SETTING_HOUR,
  SETTING_MINUTE,
  SETTING_WEIGHT,
  SAVING
};

SettingState settingState = NOT_SETTING;
int   tempHour   = 0;
int   tempMinute = 0;
float tempWeight = 0;

// ---- Manual feeding mode ----
bool  manualMode        = false;  // true if current feeding is manual
float manualTempWeight  = 100;    // default manual amount when choosing (g)
float currentTargetWeight = 0;    // unified target for scheduled + manual

enum ManualState {
  MANUAL_IDLE,
  MANUAL_SET_WEIGHT
};

ManualState manualState = MANUAL_IDLE;

// ---- Debounce ----
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

// ---- Trigger guard (fire once per Y/M/D/H/M) ----
int lastTriggerYear = -1, lastTriggerMonth = -1, lastTriggerDay = -1,
    lastTriggerHour = -1, lastTriggerMinute = -1;

// ---- Forward decls ----
void updateDisplay();
void handleSettingMode();
void adjustSettingValue(int direction);
void saveCurrentSlot();
void checkScheduledFeeding();
void startFeeding(int slotIndex);
void startManualFeeding(float weight);
void openFeeder();
void closeFeeder();
void monitorFeeding();
void finishFeeding();
DateTime getNextFeedingTime();
void resetSystemState();

// Forward declarations for API handlers
void handleStatusApi();
void handleManualFeedApi();
void handleSetSlotApi();
void handleResetApi();

// === OPTION A: weight source wrapper ===
// Returns either simulated weight (Wokwi) or real HX711 reading (hardware)
float readWeight(bool feedingActive, bool feederOpen) {
#if SIM_FAKE_WEIGHT
  // In Wokwi: simulate bowl weight increase while feeding
  if (feedingActive && feederOpen) {
    static unsigned long last = 0;
    if (millis() - last > 300) {  // every 0.3s
      simWeight += 10.0f;         // +10 g per step
      last = millis();
    }
  }
  // After feeding, simWeight stays at the final value.
  return simWeight;
#else
  // On real hardware: read actual HX711 units
  return scale.get_units(5);
#endif
}

void setup() {
  Serial.begin(115200);

  // Match your wiring (SDA=21, SCL=22)
  Wire.begin(21, 22);

  pinMode(BUTTON_DISPLAY, INPUT_PULLUP);
  pinMode(BUTTON_SETTING, INPUT_PULLUP);
  pinMode(BUTTON_UP,      INPUT_PULLUP);
  pinMode(BUTTON_DOWN,    INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Pet Feeder v2.0");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  rtc_ok = rtc.begin();
  if (!rtc_ok) {
    Serial.println("RTC not found! (check 5V & I2C)");
    lcd.setCursor(0, 2);
    lcd.print("RTC not found!");
  } else if (!rtc.isrunning()) {
    Serial.println("RTC not running, setting compile time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  feedServo.attach(SERVO_PIN);
  feedServo.write(servoCloseAngle);

  scale.begin(HX711_DT_PIN, HX711_SCK_PIN);
  delay(200); // small settle
  scale.set_scale(calibration_factor);
  scale.tare();

    // --- WiFi setup (Wokwi) ---
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());


    // HTTP server routes
  server.on("/", HTTP_GET, []() {
  server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/api/status", HTTP_GET, handleStatusApi);
  server.on("/api/manual-feed", HTTP_POST, handleManualFeedApi);
  server.on("/api/set-slot", HTTP_POST, handleSetSlotApi);
  server.on("/api/reset", HTTP_POST, handleResetApi);

  server.begin();
  Serial.println("HTTP server started.");


  delay(800);
  lcd.clear();

  Serial.println("Pet Feeding System Ready!");
  Serial.println("RED=Display | GREEN=Setting/Manual | BLUE UP/DOWN=Navigate");


}

void loop() {
  server.handleClient();

  DateTime now = rtc_ok ? rtc.now()
                        : DateTime(2025, 1, 1, 0, 0, (millis()/1000) % 60);

  // Use wrapper (sim or real)
  currentWeight = readWeight(feedingActive, feederOpen);

  // Simple debounce guard
  if (millis() - lastButtonPress < debounceDelay) return;

  // --- BUTTON HANDLING ---

  // RED: display toggle (main <-> slots)
  if (digitalRead(BUTTON_DISPLAY) == LOW) {
    if (settingState == NOT_SETTING && manualState == MANUAL_IDLE) {
      showSlots = !showSlots;
      updateDisplay();
      Serial.print("Display mode: ");
      Serial.println(showSlots ? "Slots" : "Main");
    }
    lastButtonPress = millis();
    return;
  }

  // GREEN: settings / manual feed
  if (digitalRead(BUTTON_SETTING) == LOW) {

    // 1) If we are currently choosing manual feed amount -> confirm & start
    if (manualState == MANUAL_SET_WEIGHT) {
      startManualFeeding(manualTempWeight);
      manualState = MANUAL_IDLE;
    }
    // 2) If on slots screen -> use normal slot setting mode
    else if (showSlots) {
      handleSettingMode();
    }
    // 3) If on main screen & not editing slots -> enter manual feed setup
    else if (settingState == NOT_SETTING && !showSlots) {
      manualState = MANUAL_SET_WEIGHT;
      if (manualTempWeight <= 0) manualTempWeight = 100; // default
      Serial.println("Manual feed setup started");
      updateDisplay();
    }
    // Fallback: normal setting handler
    else {
      handleSettingMode();
    }

    lastButtonPress = millis();
    return;
  }

  // UP
  if (digitalRead(BUTTON_UP) == LOW) {

    if (manualState == MANUAL_SET_WEIGHT) {
      manualTempWeight += 10;           // +10g per press
      if (manualTempWeight > 5000) manualTempWeight = 5000; // cap at 5kg
      updateDisplay();
    }
    else if (settingState == NOT_SETTING && showSlots) {
      currentSlot = (currentSlot - 1 + 3) % 3;
      updateDisplay();
      Serial.printf("UP - Selected slot: %d\n", currentSlot + 1);
    }
    else if (settingState != NOT_SETTING) {
      adjustSettingValue(1);
      updateDisplay();
    }

    lastButtonPress = millis();
    return;
  }

  // DOWN
  if (digitalRead(BUTTON_DOWN) == LOW) {

    if (manualState == MANUAL_SET_WEIGHT) {
      manualTempWeight -= 10;       // -10g per press
      if (manualTempWeight < 0) manualTempWeight = 0;
      updateDisplay();
    }
    else if (settingState == NOT_SETTING && showSlots) {
      currentSlot = (currentSlot + 1) % 3;
      updateDisplay();
      Serial.printf("DOWN - Selected slot: %d\n", currentSlot + 1);
    }
    else if (settingState != NOT_SETTING) {
      adjustSettingValue(-1);
      updateDisplay();
    }

    lastButtonPress = millis();
    return;
  }

  // --- Main logic ---
  if (settingState == NOT_SETTING && manualState == MANUAL_IDLE) {
    checkScheduledFeeding();
  }

  if (feedingActive) {
    monitorFeeding();
  }

  static unsigned long lastScreenUpdate = 0;
  if (millis() - lastScreenUpdate > 1000) {
    if (settingState == NOT_SETTING && manualState == MANUAL_IDLE) {
      updateDisplay();
    }
    lastScreenUpdate = millis();
  }

  delay(50);
}

// --- Scheduled feeding check ---
void checkScheduledFeeding() {
  if (feedingActive) return;

  DateTime now = rtc_ok ? rtc.now()
                        : DateTime(2025, 1, 1, 0, 0, (millis()/1000) % 60);

  for (int i = 0; i < 3; i++) {
    if (slots[i].active && slots[i].weight > 0) {
      if (now.hour() == slots[i].hour &&
          now.minute() == slots[i].minute &&
          now.second() <= 1) { // tolerate 0/1 sec

        // Fire once per minute
        if (!(lastTriggerYear   == now.year()  &&
              lastTriggerMonth  == now.month() &&
              lastTriggerDay    == now.day()   &&
              lastTriggerHour   == now.hour()  &&
              lastTriggerMinute == now.minute())) {

          lastTriggerYear   = now.year();
          lastTriggerMonth  = now.month();
          lastTriggerDay    = now.day();
          lastTriggerHour   = now.hour();
          lastTriggerMinute = now.minute();

          startFeeding(i);
          break;
        }
      }
    }
  }
}

// --- Start scheduled feeding ---
void startFeeding(int slotIndex) {
  manualMode = false;                 // this is a scheduled feed
  feedingActive = true;
  feederOpen = false;
  activeFeedingSlot = slotIndex;

  // ✅ Scheduled feed also "adds" on top of existing bowl weight
  currentTargetWeight = fabs(currentWeight) + slots[slotIndex].weight;

  feedingStartMs = millis();
  lastWeightDuringFeed = fabs(currentWeight);
  lastWeightChangeMs = millis();

  Serial.printf("Feeding started from SLOT%d\n", slotIndex + 1);
  Serial.printf("Target weight: %.0fg\n", currentTargetWeight);

  openFeeder();
  updateDisplay();
}

// --- Start manual feeding ---
void startManualFeeding(float weight) {
  manualMode = true;                 // manual feed
  feedingActive = true;
  feederOpen = false;
  activeFeedingSlot = -1;            // no slot associated

  // ✅ Manual feed is "add this much more":
  //     target = current bowl weight + requested extra
  currentTargetWeight = fabs(currentWeight) + weight;

  feedingStartMs = millis();
  lastWeightDuringFeed = fabs(currentWeight);
  lastWeightChangeMs = millis();

  Serial.println("Manual feeding started");
  Serial.printf("Manual target: %.0fg (current %.1f + %.1f)\n",
                currentTargetWeight, currentWeight, weight);

  openFeeder();
  updateDisplay();
}


void openFeeder() {
  feedServo.write(servoOpenAngle);
  feederOpen = true;
  Serial.println("Feeder opened (180 deg)");
}

void closeFeeder() {
  feedServo.write(servoCloseAngle);
  feederOpen = false;
  Serial.println("Feeder closed (0 deg)");
}

// --- Feeding monitor (scheduled + manual) ---
void monitorFeeding() {
  if (!feedingActive) return;

  float target = currentTargetWeight;
  float w = fabs(currentWeight);

  // Close when target reached
  if (feederOpen && w >= target && target > 0) {
    Serial.printf("Target reached: %.1fg >= %.1fg\n", w, target);
    closeFeeder();
    finishFeeding();
    return;
  }

  // Stuck detection: weight not increasing enough while open
  if (feederOpen) {
    if (w > lastWeightDuringFeed + MIN_INCREASE_G) {
      lastWeightDuringFeed = w;
      lastWeightChangeMs = millis();
    }
    if (millis() - lastWeightChangeMs > STUCK_WINDOW_MS) {
      Serial.println("No weight increase detected → stopping (stuck?)");
      closeFeeder();
      finishFeeding();
      return;
    }
  }

  // Safety timeout
  if (millis() - feedingStartMs > FEED_TIMEOUT_MS) {
    Serial.println("Feed timeout reached → stopping");
    if (feederOpen) closeFeeder();
    finishFeeding();
    return;
  }

  // Occasional log
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 3000) {
    Serial.printf("Feeding... %.1fg / %.1fg (t+%lus)\n",
                  w, target, (millis() - feedingStartMs)/1000);
    lastPrint = millis();
  }
}

void finishFeeding() {
  // Log BEFORE we reset manualMode / activeFeedingSlot
  bool wasManual = manualMode;
  int  slot      = activeFeedingSlot;
  float target   = currentTargetWeight;
  float finalW   = fabs(currentWeight);

  addFeedLog(wasManual, slot, target, finalW);

  feedingActive = false;
  feederOpen = false;
  manualMode = false;
  activeFeedingSlot = -1;

  Serial.println("Feeding complete!");

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("  Feeding Complete!");
  delay(3000);
}

void resetSystemState() {
  feedingActive = false;
  feederOpen = false;
  manualMode = false;
  activeFeedingSlot = -1;
  manualState = MANUAL_IDLE;
  settingState = NOT_SETTING;
  showSlots = false;

  currentTargetWeight = 0;
  lastWeightDuringFeed = 0;
  currentWeight = 0;
  feedingStartMs = millis();
  lastWeightChangeMs = millis();

  lastTriggerYear = -1;
  lastTriggerMonth = -1;
  lastTriggerDay = -1;
  lastTriggerHour = -1;
  lastTriggerMinute = -1;

  slots[0] = {false, 8, 0, 0};
  slots[1] = {false, 12, 0, 0};
  slots[2] = {false, 18, 0, 0};

  feedLogCount = 0;
  for (int i = 0; i < MAX_FEED_LOGS; ++i) {
    feedLog[i].used = false;
  }

#if SIM_FAKE_WEIGHT
  simWeight = 0.0f;
#else
  scale.tare();
#endif

  closeFeeder();
  updateDisplay();
}

void handleSettingMode() {
  if (showSlots && settingState == NOT_SETTING) {
    settingState = SETTING_HOUR;
    tempHour   = slots[currentSlot].hour;
    tempMinute = slots[currentSlot].minute;
    tempWeight = slots[currentSlot].weight;
    Serial.println("Setting mode started - Hour");
  } else if (settingState != NOT_SETTING) {
    switch (settingState) {
      case SETTING_HOUR:
        settingState = SETTING_MINUTE;
        Serial.println("Setting minute");
        break;
      case SETTING_MINUTE:
        settingState = SETTING_WEIGHT;
        Serial.println("Setting weight");
        break;
      case SETTING_WEIGHT:
        settingState = SAVING;
        saveCurrentSlot();
        Serial.println("Settings saved");
        break;
      case SAVING:
        settingState = NOT_SETTING;
        Serial.println("Setting mode ended");
        break;
    }
  }
  updateDisplay();
}

void adjustSettingValue(int direction) {
  switch (settingState) {
    case SETTING_HOUR:
      tempHour += direction;
      if (tempHour < 0) tempHour = 23;
      if (tempHour > 23) tempHour = 0;
      break;
    case SETTING_MINUTE:
      tempMinute += direction;
      if (tempMinute < 0) tempMinute = 59;
      if (tempMinute > 59) tempMinute = 0;
      break;
    case SETTING_WEIGHT:
      tempWeight += direction * 100;     // step by 100g
      if (tempWeight < 0)    tempWeight = 0;
      if (tempWeight > 9999) tempWeight = 9999;
      break;
    default:
      break;
  }
}

void saveCurrentSlot() {
  slots[currentSlot].hour   = tempHour;
  slots[currentSlot].minute = tempMinute;
  slots[currentSlot].weight = tempWeight;
  slots[currentSlot].active = (tempWeight > 0);

  Serial.printf("Slot %d saved: %02d:%02d, %.0fg\n",
                currentSlot + 1, tempHour, tempMinute, tempWeight);
}

DateTime getNextFeedingTime() {
  DateTime now = rtc_ok ? rtc.now()
                        : DateTime(2025, 1, 1, 0, 0, (millis()/1000) % 60);
  DateTime nextFeed(2099, 12, 31, 23, 59, 0);

  for (int i = 0; i < 3; i++) {
    if (slots[i].active && slots[i].weight > 0) {
      DateTime t(now.year(), now.month(), now.day(),
                 slots[i].hour, slots[i].minute, 0);
      if (t < now) t = t + TimeSpan(1, 0, 0, 0);
      if (t < nextFeed) nextFeed = t;
    }
  }
  return nextFeed;
}

void updateDisplay() {
  DateTime now = rtc_ok ? rtc.now()
                        : DateTime(2025, 1, 1, 0, 0, (millis()/1000) % 60);
  lcd.clear();

  // --- Manual feeding weight selection screen ---
  if (manualState == MANUAL_SET_WEIGHT) {
    lcd.setCursor(0, 0);
    lcd.print("  Manual Feeding  ");

    lcd.setCursor(0, 1);
    lcd.print("Amount: ");
    lcd.print((int)manualTempWeight);
    lcd.print("g   ");

    lcd.setCursor(0, 2);
    lcd.print("UP/DOWN: adjust");

    lcd.setCursor(0, 3);
    lcd.print("GREEN: start feed");
    return;  // don't draw other screens
  }

  if (settingState != NOT_SETTING) {
    lcd.setCursor(0, 0);
    lcd.print("Setting SLOT");
    lcd.print(currentSlot + 1);

    lcd.setCursor(0, 1);
    lcd.print("Hour: ");
    if (tempHour < 10) lcd.print("0");
    lcd.print(tempHour);
    if (settingState == SETTING_HOUR) lcd.print(" <--");

    lcd.setCursor(0, 2);
    lcd.print("Min:  ");
    if (tempMinute < 10) lcd.print("0");
    lcd.print(tempMinute);
    if (settingState == SETTING_MINUTE) lcd.print(" <--");

    lcd.setCursor(0, 3);
    lcd.print("Weight: ");
    lcd.print((int)tempWeight);
    lcd.print("g");
    if (settingState == SETTING_WEIGHT) lcd.print(" <--");

    if (settingState == SAVING) {
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("  Settings Saved!");
      lcd.setCursor(0, 2);
      lcd.print("  Press GREEN");
    }

  } else if (showSlots) {
    lcd.setCursor(0, 0);
    lcd.print("SLOTS   ");
    if (now.hour() < 10) lcd.print("0");
    lcd.print(now.hour());
    lcd.print(":");
    if (now.minute() < 10) lcd.print("0");
    lcd.print(now.minute());

    for (int i = 0; i < 3; i++) {
      lcd.setCursor(0, i + 1);
      lcd.print(i == currentSlot ? ">" : " ");
      lcd.print("SLOT");
      lcd.print(i + 1);
      lcd.print(":");
      if (slots[i].active && slots[i].weight > 0) {
        if (slots[i].hour < 10) lcd.print("0");
        lcd.print(slots[i].hour);
        lcd.print(":");
        if (slots[i].minute < 10) lcd.print("0");
        lcd.print(slots[i].minute);
        lcd.print(",");
        lcd.print((int)slots[i].weight);
        lcd.print("g");
      } else {
        lcd.print("Empty");
      }
    }

  } else {
    // --- Main screen ---
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    if (now.hour() < 10) lcd.print("0");
    lcd.print(now.hour());
    lcd.print(":");
    if (now.minute() < 10) lcd.print("0");
    lcd.print(now.minute());
    lcd.print(":");
    if (now.second() < 10) lcd.print("0");
    lcd.print(now.second());

    lcd.setCursor(0, 1);
    lcd.print("Weight: ");
    lcd.print(currentWeight, 1);
    lcd.print("g");

    if (feedingActive) {
      lcd.setCursor(0, 2);
      lcd.print("Feeding in progress");
      lcd.setCursor(0, 3);
      if (manualMode) {
        lcd.print("Manual Target: ");
      } else {
        lcd.print("Target: ");
      }
      lcd.print((int)currentTargetWeight);
      lcd.print("g");
    } else {
      lcd.setCursor(0, 2);
      lcd.print("Next: ");
      DateTime nextFeed = getNextFeedingTime();
      if (nextFeed.hour() < 10) lcd.print("0");
      lcd.print(nextFeed.hour());
      lcd.print(":");
      if (nextFeed.minute() < 10) lcd.print("0");
      lcd.print(nextFeed.minute());

      lcd.setCursor(0, 3);
      lcd.print("GREEN: Manual feed");
    }
  }
}

void addFeedLog(bool manual, int slotIndex, float target, float finalWeight) {
  // Shift older entries down (newest at index 0)
  for (int i = MAX_FEED_LOGS - 1; i > 0; --i) {
    feedLog[i] = feedLog[i - 1];
  }

  DateTime now = rtc_ok ? rtc.now()
                        : DateTime(2025, 1, 1, 0, 0, (millis()/1000) % 60);

  feedLog[0].used        = true;
  feedLog[0].manual      = manual;
  feedLog[0].slotIndex   = slotIndex;
  feedLog[0].hour        = now.hour();
  feedLog[0].minute      = now.minute();
  feedLog[0].target      = target;
  feedLog[0].finalWeight = finalWeight;

  if (feedLogCount < MAX_FEED_LOGS) {
    feedLogCount++;
  }
}


void handleStatusApi() {
  DateTime nextFeed = getNextFeedingTime();

  String json = "{";

  // weight
  json += "\"weight\":" + String(currentWeight, 1) + ",";

  // feedingActive
  json += "\"feedingActive\":" + String(feedingActive ? "true" : "false") + ",";

  // nextTime
  if (nextFeed.year() >= 2099) {
    json += "\"nextTime\":\"None\",";
  } else {
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", nextFeed.hour(), nextFeed.minute());
    json += "\"nextTime\":\"" + String(buf) + "\",";
  }

  // slots array
  json += "\"slots\":[";
  for (int i = 0; i < 3; i++) {
    json += "{";
    json += "\"active\":" + String(slots[i].active ? "true" : "false") + ",";
    json += "\"hour\":"   + String(slots[i].hour) + ",";
    json += "\"minute\":" + String(slots[i].minute) + ",";
    json += "\"weight\":" + String((int)slots[i].weight);
    json += "}";
    if (i < 2) json += ",";
  }
  json += "],";

  // 🔹 history array (new)
  json += "\"history\":[";
  for (int i = 0; i < feedLogCount; i++) {
    FeedLogEntry &e = feedLog[i];
    if (!e.used) continue;

    json += "{";

    // time "HH:MM"
    char tbuf[6];
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d", e.hour, e.minute);
    json += "\"time\":\"" + String(tbuf) + "\",";

    // type: "Manual" or "Slot X"
    json += "\"type\":\"";
    if (e.manual) {
      json += "Manual";
    } else {
      json += "Slot ";
      json += String(e.slotIndex + 1);
    }
    json += "\",";

    // target / final
    json += "\"target\":" + String((int)e.target) + ",";
    json += "\"final\":"  + String((int)e.finalWeight);

    json += "}";
    if (i < feedLogCount - 1) json += ",";
  }
  json += "]";

  json += "}";

  server.send(200, "application/json", json);
}

void handleResetApi() {
  resetSystemState();
  server.send(200, "text/plain", "OK");
}


void handleManualFeedApi() {
  if (!server.hasArg("amount")) {
    server.send(400, "text/plain", "Missing amount");
    return;
  }

  float amount = server.arg("amount").toFloat();
  if (amount <= 0) {
    server.send(400, "text/plain", "Amount must be > 0");
    return;
  }

  if (feedingActive) {
    server.send(409, "text/plain", "Already feeding");
    return;
  }

  startManualFeeding(amount);  // your existing function
  server.send(200, "text/plain", "OK");
}

void handleSetSlotApi() {
  if (!server.hasArg("index") ||
      !server.hasArg("hour")  ||
      !server.hasArg("minute")||
      !server.hasArg("weight")) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }

  int index  = server.arg("index").toInt();
  int hour   = server.arg("hour").toInt();
  int minute = server.arg("minute").toInt();
  float weight = server.arg("weight").toFloat();

  if (index < 0 || index > 2) {
    server.send(400, "text/plain", "Invalid slot index");
    return;
  }
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    server.send(400, "text/plain", "Invalid time");
    return;
  }
  if (weight < 0) weight = 0;

  slots[index].hour   = hour;
  slots[index].minute = minute;
  slots[index].weight = weight;
  slots[index].active = (weight > 0);

  Serial.printf("Slot %d set via Web: %02d:%02d, %.0fg\n",
                index + 1, hour, minute, weight);

  server.send(200, "text/plain", "OK");
}

