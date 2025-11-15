/*********************************************************************
  ESP32 + ST7796U  →  Digital Clock
  Requires: TFT_eSPI (configured for ST7796U 8-bit parallel)
*********************************************************************/

#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>

// ---------- USER SETTINGS ----------
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

bool use24h = true;          // false → 12h AM/PM  true → 24h
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;      // CET = UTC+0 - add 3600 per UTC+ (read readme for more info)
const int   daylightOffset_sec = 0; // CEST = UTC+0 (handled automatically)
// -----------------------------------

TFT_eSPI tft = TFT_eSPI();

unsigned long lastSync = 0;
const unsigned long syncInterval = 12UL * 60UL * 60UL * 1000UL; // 12 h

// Font sizes (built-in)
#define TIME_FONT   &FreeSansBold24pt7b
#define DATE_FONT   &FreeSansBold12pt7b
#define SEC_FONT    &FreeSansBold9pt7b

void setup() {
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1);               // landscape
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  connectWiFi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  waitForNTP();
  drawStaticUI();
}

void loop() {
  static unsigned long lastSec = 0;
  unsigned long now = millis();

  // Sync NTP every 12 h
  if (now - lastSync > syncInterval) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    lastSync = now;
  }

  // Update once per second
  if (now - lastSec >= 1000) {
    lastSec = now;
    updateClock();
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  tft.setCursor(10, 100);
  tft.print("Connecting WiFi...");
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 100);
    tft.print("WiFi OK");
    delay(800);
  } else {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 100);
    tft.print("WiFi FAILED");
    while (true) delay(1000);
  }
}

void waitForNTP() {
  struct tm timeinfo;
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 100);
  tft.print("Syncing NTP...");
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 30) {
    delay(500);
    attempts++;
  }
  if (attempts >= 30) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 100);
    tft.print("NTP FAILED");
    while (true) delay(1000);
  }
}

void drawStaticUI() {
  tft.fillScreen(TFT_BLACK);

  // Date line (top)
  tft.setTextDatum(TC_DATUM); // top-center
  tft.setFreeFont(DATE_FONT);
  tft.drawString("DATE", tft.width()/2, 10);

  // Time line (center)
  tft.setFreeFont(TIME_FONT);
  tft.setTextDatum(MC_DATUM); // middle-center
  // placeholder
  tft.drawString("88:88", tft.width()/2, tft.height()/2 - 20);

  // Seconds bar (bottom)
  tft.drawRect(20, tft.height()-50, tft.width()-40, 20, TFT_CYAN);
}

void updateClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  char buf[32];

  // ---- DATE ----
  strftime(buf, sizeof(buf), "%a %d %b %Y", &timeinfo);
  tft.setFreeFont(DATE_FONT);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(buf, tft.width()/2, 40);

  // ---- TIME ----
  int h = timeinfo.tm_hour;
  int m = timeinfo.tm_min;
  int s = timeinfo.tm_sec;

  if (!use24h) {
    bool pm = (h >= 12);
    if (h > 12) h -= 12;
    if (h == 0) h = 12;
    snprintf(buf, sizeof(buf), "%d:%02d%s", h, m, pm ? " PM" : " AM");
  } else {
    snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
  }

  tft.setFreeFont(TIME_FONT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(buf, tft.width()/2, tft.height()/2 - 20);

  // ---- SECONDS BAR ----
  int barW = tft.width() - 40;
  int fill = map(s, 0, 59, 0, barW);
  tft.fillRect(20, tft.height()-50, barW, 20, TFT_BLACK); // clear
  tft.fillRect(20, tft.height()-50, fill, 20, TFT_CYAN);

  // optional small seconds number
  snprintf(buf, sizeof(buf), "%02d", s);
  tft.setFreeFont(SEC_FONT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(buf, tft.width()/2, tft.height()-30);
}
