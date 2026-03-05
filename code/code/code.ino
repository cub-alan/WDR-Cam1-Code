#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Gps.hpp"
#include "Cam.hpp"
#include "OTA.hpp"

const char* ssid = "iPhone";
const char* password = "12345678";

void setup() {

  setCpuFrequencyMhz(80);

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  OTA_Init();
  delay(2000);
  
  Gnss_init();
  delay(2000);
  Cam_init();
  delay(2000);

  printf("\033[?25l"); // hide the cursor

  int DotCount = 0;
  Serial.print("\033[1;1H Wifi Connecting");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (DotCount < 3){
      Serial.print(".");
      DotCount++;
    }
    else{
      Serial.print("\033[1;17H\033[K"); 
      DotCount = 0;
    }
  }
    Serial.println("\033[1;1H WIFI CONNECTED: ");
    Serial.print("Stream URL: http://");
    Serial.println(WiFi.localIP());

    // Start the MJPEG streaming server on port 80
    Server_init();

}

void loop() {
  OTA_Handle();
  static int DotCount = 0;
  if (xSemaphoreTake(GPS.mutex, (TickType_t)10) == pdTRUE) {
    if (GPS.val) {
        Serial.printf("\033[3;1H LAT: %.6f, LON: %.6f, ALT: %.2f m\n", GPS.lat, GPS.lon, GPS.alt);
    }
    else {
      Serial.print("\033[3;1H GPS STATUS: Connecting");
      for(int i=0; i<DotCount; i++) Serial.print(".");
      Serial.print("\033[K");
      DotCount++;
      if (DotCount > 3) DotCount = 0;
    }
    xSemaphoreGive(GPS.mutex);
  }
  delay(2000);
}
