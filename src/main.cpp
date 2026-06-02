/***************************************************************************
 *  Cave a vin - Firmware ESP32 D1 Mini (PlatformIO / VS Code)
 *  PN532 (I2C) + SH1106 1.3" (I2C) + BME280 (I2C) + Ruban SK6812 RGBW
 *  + LDR deep sleep + MOSFET AO3401 + 2 boutons + WiFi
 ***************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <esp_sleep.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_BME280.h>
#include <Adafruit_PN532.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "settings.h"
#include "portal.h"
#include "lang.h"

// ============ OBJETS GLOBAUX ============
Adafruit_SH1106G    oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
Adafruit_PN532      nfc(-1, -1);
Adafruit_BME280     bme;
Adafruit_NeoPixel   strip(LED_COUNT, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);

bool bmeOk = false;
bool nfcOk = false;

// Etat de la derniere lecture
String        lastUid        = "";
String        lastName       = "";
String        lastMillesime  = "";
int           lastQty        = 0;
bool          lastFound      = false;
unsigned long lastReadAt  = 0;
unsigned long lastEnvSend = 0;

// Mode enregistrement
bool          enrollMode    = false;
unsigned long enrollStartAt = 0;

// Boutons
unsigned long btnPlusDownAt   = 0;
unsigned long btnMinusDownAt  = 0;
bool          btnPlusHandled  = false;
bool          btnMinusHandled = false;

// LDR / veille
unsigned long darkSince      = 0;
unsigned long lastActivityAt = 0;

// ============ PROTOTYPES ============
void   setupWifi();
void   showMsg(const String& l1, const String& l2 = "", const String& l3 = "");
void   showWine(const String& name, const String& millesime, int qty, const String& hint = "");
void   showEnroll(const String& subtitle = "");
String uidToString(uint8_t* uid, uint8_t uidLength);
String httpCall(const String& url);
String buildUrl(const String& action, const String& params = "");
bool   callApi(const String& url, JsonDocument& doc);
void   doLookup(const String& uid);
void   doUpdate(int delta);
void   doRegister(const String& uid);
void   sendEnvironment();
void   enterEnrollMode();
void   exitEnrollMode(const String& reason = "");
void   doTimerMeasure();
void   handleButtons();
void   applyBtnOverlay();
void   redrawCurrentScreen();
void   updateBtnVisual();
void   checkSleep();
void   goToSleep();

// ============ UTILS ============
String uidToString(uint8_t* uid, uint8_t uidLength) {
  String s = "";
  for (uint8_t i = 0; i < uidLength; i++) {
    if (uid[i] < 0x10) s += "0";
    s += String(uid[i], HEX);
  }
  s.toUpperCase();
  return s;
}

// ============ LED STRIP ============
void ledsOff() {
  for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, 0);
  strip.show();
}

void ledsColor(uint32_t c) {
  for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, c);
  strip.show();
}

void ledsFlash(uint32_t color, int n) {
  for (int i = 0; i < n; i++) {
    ledsColor(color);
    delay(120);
    ledsOff();
    delay(80);
  }
}

void updateLeds() {
  static unsigned long last = 0;
  static float angle = 0.0f;
  if (millis() - last < 30) return;
  last = millis();

  float val = (sinf(angle) + 1.0f) * 0.5f;
  uint8_t b  = (uint8_t)(val * 80.0f + 10.0f);
  uint32_t color = enrollMode
    ? strip.Color(b / 2, 0, b)
    : strip.Color(0, 0, b);
  ledsColor(color);

  angle += 0.06f;
  if (angle > 6.2832f) angle -= 6.2832f;
}

void flashFeedback() {
  for (int i = 0; i < 2; i++) {
    ledsColor(strip.Color(255, 255, 255));
    delay(80);
    ledsOff();
    delay(80);
  }
}

// ============ AFFICHAGE OLED ============
void showMsg(const String& l1, const String& l2, const String& l3) {
  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);   oled.println(l1);
  oled.setCursor(0, 20);  oled.println(l2);
  oled.setCursor(0, 40);  oled.println(l3);
  applyBtnOverlay();
  oled.display();
}

void showWine(const String& name, const String& millesime, int qty, const String& hint) {
  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println(T->wineScanned);
  String n = name;
  if (n.length() > 21) n = n.substring(0, 21);
  oled.setCursor(0, 10);
  oled.println(n);
  oled.setCursor(0, 20);
  oled.print(millesime.length() ? millesime : "-");
  oled.setTextSize(2);
  oled.setCursor(0, 32);
  oled.print(T->qty);
  oled.print(qty);
  oled.setTextSize(1);
  oled.setCursor(0, 56);
  oled.print(hint);
  applyBtnOverlay();
  oled.display();
}

void showEnroll(const String& subtitle) {
  String sub = subtitle.length() > 0 ? subtitle : String(T->placeLabel);
  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println(T->enrollTitle);
  oled.setCursor(0, 20);
  oled.println(sub);
  oled.setCursor(0, 44);
  oled.println(T->longMinusCancel);
  applyBtnOverlay();
  oled.display();
}

// ============ HTTP ============
String httpCall(const String& url) {
  if (WiFi.status() != WL_CONNECTED) return "";
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_ANON_KEY));
  int code = http.GET();
  String body = "";
  if (code == 200) body = http.getString();
  http.end();
  return body;
}

String buildUrl(const String& action, const String& params) {
  String u = String(SUPABASE_URL);
  u += "?token=" + settings.deviceToken;
  u += "&action=" + action;
  if (params.length()) u += "&" + params;
  return u;
}

bool callApi(const String& url, JsonDocument& doc) {
  String body = httpCall(url);
  if (body.length() == 0) return false;
  return !deserializeJson(doc, body);
}

// ============ ACTIONS ============
void showWineCountdown(const String& name, const String& millesime, int qty, int sec) {
  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);
  oled.setTextSize(1);
  String n = name;
  if (n.length() > 21) n = n.substring(0, 21);
  oled.setCursor(0, 0);
  oled.println(n);
  oled.setCursor(0, 10);
  oled.print(millesime.length() ? millesime : "-");
  oled.setCursor(0, 22);
  oled.print(T->qty);
  oled.print(qty);
  oled.setTextSize(2);
  String cs = String(sec) + "s";
  oled.setCursor(128 - (int)cs.length() * 12, 20);
  oled.print(cs);
  oled.setTextSize(1);
  oled.setCursor(0, 56);
  oled.print(T->cancelMinus);
  applyBtnOverlay();
  oled.display();
}

void doLookup(const String& uid) {
  showMsg(T->searching, uid);
  StaticJsonDocument<512> doc;
  if (!callApi(buildUrl("lookup", "uid=" + uid), doc)) {
    showMsg(T->networkError, uid);
    ledsFlash(strip.Color(255, 0, 0), 2);
    return;
  }
  if (!doc["ok"].as<bool>()) {
    showMsg(T->apiError, doc["error"] | "");
    ledsFlash(strip.Color(255, 0, 0), 2);
    return;
  }
  bool found = doc["found"] | false;
  lastFound     = found;
  lastUid       = uid;
  lastName      = found ? String((const char*)(doc["nom"] | "?"))
                        : String(T->notRegistered);
  lastMillesime = found ? doc["millesime"].as<String>() : String("");
  lastQty       = doc["qte"] | 0;
  lastReadAt    = millis();

  if (found && lastQty > 0) {
    // Vin present : compte a rebours puis decrement automatique
    ledsFlash(strip.Color(0, 255, 0), 2);
    unsigned long deadline = millis() + EDIT_WINDOW_MS;
    int lastSec = -1;
    bool doDecrement = true;
    while (millis() < deadline) {
      int sec = (int)((deadline - millis() + 999) / 1000);
      if (sec != lastSec) {
        lastSec = sec;
        showWineCountdown(lastName, lastMillesime, lastQty, sec);
      }
      if (touchRead(PIN_BTN_MINUS) < TOUCH_THRESHOLD) {
        while (touchRead(PIN_BTN_MINUS) < TOUCH_THRESHOLD) delay(10);
        doDecrement = false;
        break;
      }
      if (touchRead(PIN_BTN_PLUS) < TOUCH_THRESHOLD) {
        while (touchRead(PIN_BTN_PLUS) < TOUCH_THRESHOLD) delay(10);
        lastActivityAt = millis();
        doUpdate(+1);
        delay(1500);
        lastUid = "";
        showMsg(T->ready, T->placeBottle, T->longPlusEnroll);
        return;
      }
      checkSleep();
      delay(50);
    }
    if (doDecrement) {
      lastActivityAt = millis();
      doUpdate(-1);
      delay(1500);
    }
  } else {
    // Vin inconnu ou stock vide : proposer d'ajouter
    if (found) {
      ledsFlash(strip.Color(255, 200, 0), 2);
      showMsg(lastName, T->plusToAdd, T->minusToIgnore);
    } else {
      ledsFlash(strip.Color(255, 128, 0), 2);
      showMsg(T->notRegistered, T->plusToAdd, T->minusToIgnore);
    }
    unsigned long deadline = millis() + EDIT_WINDOW_MS;
    while (millis() < deadline) {
      if (touchRead(PIN_BTN_PLUS) < TOUCH_THRESHOLD) {
        while (touchRead(PIN_BTN_PLUS) < TOUCH_THRESHOLD) delay(10);
        lastActivityAt = millis();
        if (found) {
          doUpdate(+1);
        } else {
          doRegister(uid);
        }
        delay(1500);
        lastUid = "";
        showMsg(T->ready, T->placeBottle, T->longPlusEnroll);
        return;
      }
      if (touchRead(PIN_BTN_MINUS) < TOUCH_THRESHOLD) {
        while (touchRead(PIN_BTN_MINUS) < TOUCH_THRESHOLD) delay(10);
        break;
      }
      checkSleep();
      delay(50);
    }
  }

  lastUid = "";
  showMsg(T->ready, T->placeBottle, T->longPlusEnroll);
}

void doUpdate(int delta) {
  if (lastUid.length() == 0) return;
  showMsg(delta > 0 ? T->adding : T->removing, lastName);
  String action = (delta > 0) ? "increment" : "decrement";
  StaticJsonDocument<384> doc;
  if (!callApi(buildUrl(action, "uid=" + lastUid), doc)) {
    showMsg(T->networkError);
    ledsFlash(strip.Color(255, 0, 0), 2);
    return;
  }
  if (!doc["ok"].as<bool>()) {
    showMsg(T->apiError, doc["error"] | "");
    ledsFlash(strip.Color(255, 0, 0), 2);
    return;
  }
  lastQty  = doc["qte"] | lastQty;
  lastName = String((const char*)(doc["nom"] | lastName.c_str()));
  ledsFlash(strip.Color(0, 255, 0), 1);
  showWine(lastName, lastMillesime, lastQty,
           delta > 0 ? T->added : T->removed);
  lastReadAt = millis();
}

void doRegister(const String& uid) {
  showEnroll(T->sending);
  StaticJsonDocument<384> doc;
  if (!callApi(buildUrl("register", "uid=" + uid), doc)) {
    showMsg(T->networkError, uid);
    ledsFlash(strip.Color(255, 0, 0), 2);
    delay(1500);
    return;
  }
  if (!doc["ok"].as<bool>()) {
    showMsg(T->apiError, doc["error"] | "");
    ledsFlash(strip.Color(255, 0, 0), 2);
    delay(2000);
    return;
  }
  bool created = doc["created"] | false;
  String nom = String((const char*)(doc["nom"] | "?"));
  if (created) {
    ledsFlash(strip.Color(0, 255, 0), 3);
    showMsg(T->registered, uid, T->completeOnDash);
  } else {
    ledsFlash(strip.Color(255, 200, 0), 2);
    showMsg(T->alreadyPresent, nom, uid);
  }
  delay(2500);
}

void sendEnvironment() {
  if (!bmeOk) return;
  float t = bme.readTemperature();
  float h = bme.readHumidity();
  float p = bme.readPressure() / 100.0f;
  String params = "t=" + String(t, 1) +
                  "&h=" + String(h, 1) +
                  "&p=" + String(p, 1);
  StaticJsonDocument<256> doc;
  callApi(buildUrl("env", params), doc);

  String alertMsg = "";
  if (t < settings.tempMin || t > settings.tempMax) alertMsg += "temperature ";
  if (h < settings.humMin  || h > settings.humMax)  alertMsg += "humidite";
  if (alertMsg.length() > 0) {
    ledsFlash(strip.Color(255, 100, 0), 4);
    showMsg(String(T->alert) + alertMsg,
            "T=" + String(t, 1) + "C  H=" + String(h, 0) + "%",
            T->pressBtnAck);
    while (true) {
      if (touchRead(PIN_BTN_PLUS) < TOUCH_THRESHOLD || touchRead(PIN_BTN_MINUS) < TOUCH_THRESHOLD) {
        while (touchRead(PIN_BTN_PLUS) < TOUCH_THRESHOLD || touchRead(PIN_BTN_MINUS) < TOUCH_THRESHOLD)
          delay(10);
        lastActivityAt = millis();
        break;
      }
      checkSleep();
      delay(50);
    }
    redrawCurrentScreen();
  }
}

// ============ MODE ENREGISTREMENT ============
void enterEnrollMode() {
  enrollMode = true;
  enrollStartAt = millis();
  lastUid = "";
  showEnroll();
}

void exitEnrollMode(const String& reason) {
  enrollMode = false;
  if (reason.length() > 0) {
    showMsg(reason);
    delay(1200);
  }
  showMsg(T->ready, T->placeBottle, T->longPlusEnroll);
}

// ============ VERIFICATION TOKEN ============
bool verifyToken() {
  if (settings.deviceToken.length() == 0) return false;
  showMsg(T->tokenChecking);
  StaticJsonDocument<128> doc;
  if (!callApi(buildUrl("ping"), doc)) return true; // API injoignable : ne pas bloquer
  return doc["ok"].as<bool>();
}

// ============ WIFI ============
void setupWifi() {
  ledsOff();
  showMsg(T->connectingWifi, settings.wifiSsid);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  WiFi.mode(WIFI_STA);
  WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPass.c_str());
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(300);
  }
  if (WiFi.status() == WL_CONNECTED) {
    ledsFlash(strip.Color(0, 255, 0), 2);
    showMsg(T->wifiOk, WiFi.localIP().toString(), FIRMWARE_VERSION);
  } else {
    ledsFlash(strip.Color(255, 0, 0), 2);
    showMsg(T->wifiFailed, T->offlineMode);
  }
  delay(1200);
}

// ============ VISUEL BOUTONS OLED ============
void applyBtnOverlay() {
  if (touchRead(PIN_BTN_PLUS) < TOUCH_THRESHOLD)
    oled.fillRect(OLED_WIDTH - 5, 0, 5, OLED_HEIGHT, SH110X_WHITE);
  if (touchRead(PIN_BTN_MINUS) < TOUCH_THRESHOLD)
    oled.fillRect(0, 0, 5, OLED_HEIGHT, SH110X_WHITE);
}

void redrawCurrentScreen() {
  if (isPortalActive()) {
    showMsg(T->configMode, PORTAL_AP_SSID, "192.168.4.1");
    return;
  }
  if (enrollMode) {
    showEnroll();
  } else if (lastUid.length() > 0 && (millis() - lastReadAt < EDIT_WINDOW_MS)) {
    showWine(lastName, lastMillesime, lastQty,
             lastFound ? T->plusMinusModify : T->longPlusRegister);
  } else {
    showMsg(T->ready, T->placeBottle, T->longPlusEnroll);
  }
}

void updateBtnVisual() {
  static bool wasPlusActive  = false;
  static bool wasMinusActive = false;
  bool plusActive  = (touchRead(PIN_BTN_PLUS)  < TOUCH_THRESHOLD);
  bool minusActive = (touchRead(PIN_BTN_MINUS) < TOUCH_THRESHOLD);
  if (plusActive != wasPlusActive || minusActive != wasMinusActive) {
    wasPlusActive  = plusActive;
    wasMinusActive = minusActive;
    redrawCurrentScreen();
  }
}

// ============ BOUTONS ============
void handleButtons() {
  bool plus  = (touchRead(PIN_BTN_PLUS)  < TOUCH_THRESHOLD);
  bool minus = (touchRead(PIN_BTN_MINUS) < TOUCH_THRESHOLD);
  unsigned long now = millis();

  static unsigned long bothDownAt  = 0;
  static bool          bothHandled = false;
  if (plus && minus) {
    if (bothDownAt == 0) { bothDownAt = now; bothHandled = false; }
    if (!bothHandled && (now - bothDownAt >= LONGPRESS_MS)) {
      bothHandled = true;
      lastActivityAt = now;
      Serial.println("BTN+/- long : demarrage portail config");
      startPortal();
    }
    return;
  } else {
    bothDownAt  = 0;
    bothHandled = false;
  }

  if (plus) {
    if (btnPlusDownAt == 0) { btnPlusDownAt = now; btnPlusHandled = false; }
    if (!btnPlusHandled && (now - btnPlusDownAt >= LONGPRESS_MS)) {
      btnPlusHandled = true;
      lastActivityAt = now;
      Serial.println("BTN+ : appui long");
      if (!enrollMode) enterEnrollMode();
    }
  } else {
    if (btnPlusDownAt != 0) {
      unsigned long held = now - btnPlusDownAt;
      btnPlusDownAt = 0;
      if (!btnPlusHandled && held >= DEBOUNCE_MS) {
        lastActivityAt = now;
      }
    }
  }

  if (minus) {
    if (btnMinusDownAt == 0) { btnMinusDownAt = now; btnMinusHandled = false; }
    if (!btnMinusHandled && (now - btnMinusDownAt >= LONGPRESS_MS)) {
      btnMinusHandled = true;
      lastActivityAt = now;
      Serial.println("BTN- : appui long");
      if (enrollMode) exitEnrollMode(T->cancelled);
    }
  } else {
    if (btnMinusDownAt != 0) {
      unsigned long held = now - btnMinusDownAt;
      btnMinusDownAt = 0;
      if (!btnMinusHandled && held >= DEBOUNCE_MS) {
        lastActivityAt = now;
      }
    }
  }
}

// ============ MESURE QUOTIDIENNE (reveil timer) ============
void doTimerMeasure() {
  if (!bmeOk) { goToSleep(); return; }

  ledsOff();
  unsigned long t0       = millis();
  unsigned long lastOled = 0;

  while (millis() - t0 < BME_WARMUP_MS) {
    unsigned long now = millis();
    if (now - lastOled >= 30000 || lastOled == 0) {
      lastOled = now;
      unsigned long rem = (BME_WARMUP_MS - (now - t0)) / 1000;
      showMsg(T->caveMeasure,
              T->bmeStab,
              String(rem / 60) + "m " + String(rem % 60) + "s");
    }
    delay(500);
  }

  setupWifi();
  sendEnvironment();
  WiFi.disconnect(true);
  delay(200);
  goToSleep();
}

// ============ VEILLE ============
void goToSleep() {
  showMsg(T->sleeping);
  for (int b = 80; b >= 0; b -= 4) {
    uint8_t v = (uint8_t)b;
    for (int i = 0; i < LED_COUNT; i++)
      strip.setPixelColor(i, strip.Color(0, 0, v));
    strip.show();
    delay(15);
  }
  ledsOff();
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_MOSFET, HIGH);

  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_LDR, 1);
  touchSleepWakeUpEnable(PIN_BTN_PLUS,  TOUCH_THRESHOLD);
  touchSleepWakeUpEnable(PIN_BTN_MINUS, TOUCH_THRESHOLD);
  esp_sleep_enable_touchpad_wakeup();
  esp_sleep_enable_timer_wakeup(ENV_DAILY_WAKEUP_US);

  Serial.println("Deep sleep...");
  delay(100);
  esp_deep_sleep_start();
}

void checkSleep() {
  static unsigned long lastLog = 0;
  int ldr = analogRead(PIN_LDR);
  unsigned long now      = millis();
  unsigned long darkMs   = (unsigned long)settings.darkTimeoutS * 1000UL;
  unsigned long inactMs  = (unsigned long)settings.inactivityTimeoutMin * 60UL * 1000UL;

  if (now - lastLog > 2000) {
    lastLog = now;
    unsigned long inactif = (now - lastActivityAt) / 1000;
    Serial.printf("LDR: %d  seuil: %d  obscur: %lus  inactif: %lus/%lus\n",
                  ldr, settings.ldrThreshold,
                  darkSince ? (now - darkSince) / 1000 : 0,
                  inactif, inactMs / 1000);
  }

  if (ldr < settings.ldrThreshold) {
    if (darkSince == 0) darkSince = now;
    if (now - darkSince > darkMs) goToSleep();
  } else {
    darkSince = 0;
  }

  if (now - lastActivityAt > inactMs) goToSleep();
}

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  loadSettings();
  setLang(settings.language);   // applique la langue sauvegardee

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  pinMode(PIN_MOSFET, OUTPUT);
  digitalWrite(PIN_MOSFET, LOW);
  pinMode(PIN_LED,      OUTPUT);
  digitalWrite(PIN_LED, LOW);

  strip.begin();
  strip.setBrightness(80);
  ledsOff();

  pinMode(PIN_I2C_SCL, OUTPUT);
  pinMode(PIN_I2C_SDA, OUTPUT);
  digitalWrite(PIN_I2C_SDA, HIGH);
  for (int i = 0; i < 9; i++) {
    digitalWrite(PIN_I2C_SCL, LOW);  delayMicroseconds(5);
    digitalWrite(PIN_I2C_SCL, HIGH); delayMicroseconds(5);
  }
  digitalWrite(PIN_I2C_SDA, LOW);  delayMicroseconds(5);
  digitalWrite(PIN_I2C_SCL, HIGH); delayMicroseconds(5);
  digitalWrite(PIN_I2C_SDA, HIGH); delayMicroseconds(5);

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if      (cause == ESP_SLEEP_WAKEUP_EXT0)      Serial.println("Reveil par LDR");
  else if (cause == ESP_SLEEP_WAKEUP_TOUCHPAD)  Serial.println("Reveil par touch");
  else if (cause == ESP_SLEEP_WAKEUP_TIMER)     Serial.println("Reveil par timer");

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setTimeOut(10);

  Serial.println("Scan I2C...");
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
      Serial.printf("  Trouve: 0x%02X\n", addr);
  }

  if (!oled.begin(OLED_ADDR, true)) {
    Serial.println("OLED absent");
  }
  oled.clearDisplay();
  oled.display();
  showMsg(T->appName, T->initializing);

  bmeOk = bme.begin(0x76) || bme.begin(0x77);
  if (!bmeOk) Serial.println("BME280 absent");

  if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    doTimerMeasure();
    return;
  }

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 absent");
  } else {
    Serial.printf("PN532 firmware: %d.%d\n",
                  (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF);
    nfc.SAMConfig();
    nfcOk = true;
  }

  lastActivityAt = millis();

  if (!hasWifiConfig()) {
    startPortal();
    return;
  }

  setupWifi();

  if (WiFi.status() == WL_CONNECTED && !verifyToken()) {
    for (int i = 0; i < 8; i++) {
      ledsColor(strip.Color(255, 0, 0));
      delay(200);
      ledsOff();
      delay(200);
    }
    showMsg(T->tokenError, PORTAL_AP_SSID, "192.168.4.1");
    delay(3000);
    startPortal();
    return;
  }

  showMsg(T->ready, T->placeBottle, T->longPlusEnroll);
}

// ============ LOOP ============
void loop() {
  if (isPortalActive()) {
    if (isPortalMsgNeeded())
      showMsg(T->configMode, PORTAL_AP_SSID, "192.168.4.1");
    handlePortal();
    updateBtnVisual();
    return;
  }

  handleButtons();

  if (nfcOk) {
    uint8_t uid[7];
    uint8_t uidLength;
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50)) {
      String uidStr = uidToString(uid, uidLength);
      lastActivityAt = millis();
      nfc.begin();
      nfc.SAMConfig();
      flashFeedback();
      if (enrollMode) {
        doRegister(uidStr);
        exitEnrollMode();
      } else {
        doLookup(uidStr);
      }
    }
  }

  if (enrollMode && (millis() - enrollStartAt > ENROLL_TIMEOUT_MS)) {
    exitEnrollMode(T->enrollTimeout);
  }

  if (millis() - lastEnvSend > ENV_PERIOD_MS) {
    lastEnvSend = millis();
    sendEnvironment();
  }

  static unsigned long lastTouchLog = 0;
  if (millis() - lastTouchLog > 500) {
    lastTouchLog = millis();
    Serial.printf("Touch T+:%d T-:%d seuil:%d\n",
                  touchRead(PIN_BTN_PLUS), touchRead(PIN_BTN_MINUS), TOUCH_THRESHOLD);
  }

  updateLeds();
  updateBtnVisual();
  checkSleep();

  delay(20);
}
