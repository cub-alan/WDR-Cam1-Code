#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Gps.hpp"
#include "Cam.hpp"
#include "OTA.hpp"
#include "Telnet.hpp"

const char* ssid = "iPhone";
const char* password = "12345678";

void setup() {

  setCpuFrequencyMhz(80);

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);

  WiFi.begin(ssid, password);
  WiFi.setSleep(true);
  WiFi.setTxPower(WIFI_POWER_8_5dBm); 

  int try = 0;

  while (WiFi.status() != WL_CONNECTED && try < 20) {
    delay(500);
    Serial.print(".");
    try++;
  }

  delay(3000);

  Gnss_init();
  delay(3000);

  Cam_init();
  delay(2000);

  OTA_Init();// On Port 81
  Telnet_Init();// On Port 23
  Server_init();// Camera on Port 80

  Serial.println("\033[1;1H SYSTEM READY");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  Wifi_Terminal_Write("\033[?25l");// hide the cursor
  Wifi_Terminal_Write("\033[2J");// clear screen
  Wifi_Terminal_Write("\033[1;1H WIFI CONNECTED\n");
  Wifi_Terminal_Write("Camera Stream: http://%s\n", WiFi.localIP().toString().c_str());
  Wifi_Terminal_Write("OTA Update: http://%s:81\n", WiFi.localIP().toString().c_str());
  Wifi_Terminal_Write("Wireless Log: telnet %s\n", WiFi.localIP().toString().c_str());
}

void loop() {
  OTA_Handle();
  static int DotCount = 0;
  if (xSemaphoreTake(GPS.mutex, (TickType_t)10) == pdTRUE) {
    if (GPS.val) {
        Wifi_Terminal_Write("\033[3;1H LAT: %.6f, LON: %.6f, ALT: %.2f m\n", GPS.lat, GPS.lon, GPS.alt);
    }
    else {
      Wifi_Terminal_Write("\033[3;1H GPS STATUS: Connecting");
      for(int i=0; i<DotCount; i++) Serial.print(".");
      Wifi_Terminal_Write("\033[K");
      DotCount++;
      if (DotCount > 3) DotCount = 0;
    }
    xSemaphoreGive(GPS.mutex);
  }
  delay(1000);
}
