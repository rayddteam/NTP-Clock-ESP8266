
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <Wire.h>

#define STRINGIFY(x) #x
constexpr char WIFI_SSID[] = STRINGIFY(ENVS_WIFI_SSID);
constexpr char WIFI_PASS[] = STRINGIFY(ENVS_WIFI_PASSWORD);

constexpr char TZ_STRING[] = "EET-2EEST,M3.5.0/3,M10.5.0/4";

constexpr char NTP1[] = "pool.ntp.org";
constexpr char NTP2[] = "time.google.com";
constexpr char NTP3[] = "time.windows.com";

#define CLK 0
#define DIO 2
#include "TM1637.h"
TM1637 TM;

uint32_t lastSyncEpoch = 0;
uint8_t  lastNtpOkFlag = 0;

constexpr uint32_t SYNC_PERIOD_MINS = 30;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 10000;
constexpr uint32_t TIME_SYNC_TIMEOUT_MS    = 15000;

bool wifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(100);
  }
  return WiFi.status() == WL_CONNECTED;
}

bool doNtpSync() {
  struct tm tmNow;
  uint32_t t0;

  t0 = millis();
  configTzTime(TZ_STRING, NTP1, NTP2, NTP3);
  while ((millis() - t0) < TIME_SYNC_TIMEOUT_MS) {
    ESP.wdtFeed();
    if (getLocalTime(&tmNow, 1000)) {
      lastNtpOkFlag = 1;
      lastSyncEpoch = (uint32_t)time(nullptr);
      return true;
    }
  }
  lastNtpOkFlag = 0;
  return false;
}

uint32_t secondsToNextMinute(const struct tm& tmNow) {
  int sec;

  sec = tmNow.tm_sec;
  return (sec == 0) ? 60 : (60 - sec);
}

bool col = true;
void drawPage(const struct tm& tmNow, bool date) {
  char sep, timeBuf[7];

  if (date) {
    sep = lastNtpOkFlag ? ':' : '.';
    snprintf(timeBuf, sizeof(timeBuf), "%02d%c%02d ", tmNow.tm_hour, sep, tmNow.tm_min);
  } else {
    strcpy(timeBuf, "--:--");
  }

  TM.begin(CLK, DIO, 4);       //  clock pin, data pin, #digits
  TM.setBitDelay(100);
  TM.setBrightness(7);
  TM.displayTime(tmNow.tm_hour, tmNow.tm_min, col);
  col = col ? false : true;
}

void setup() {
  uint32_t secToNext = 60;
  struct tm tmNow;

  setenv("TZ", TZ_STRING, 1);
  tzset();

  bool haveRTC = getLocalTime(&tmNow, 250);
  // Update screen for the case NTP will take longer.
  drawPage(tmNow, haveRTC);

  if (wifiConnect()) {
    doNtpSync();
  } else {
    lastNtpOkFlag = 0;
  }

  if (getLocalTime(&tmNow, 250)) {
    drawPage(tmNow, true);
    secToNext = secondsToNextMinute(tmNow);
    if (secToNext < 5) secToNext += 60;
  } else {
    drawPage(tmNow, false);
  }
}

void loop() {
  struct tm tmNow;
  int i;

  getLocalTime(&tmNow, 250);
  drawPage(tmNow, true);

  for (i = 0; i < 500000; i++)
  {
    if (i % 1000 == 0)
    {
      ESP.wdtFeed();
      yield();
    }
  }
}

