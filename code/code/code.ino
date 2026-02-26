#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Gps.hpp"
#include "Cam.hpp"

const char* ssid = "iPhone";
const char* password = "12345678";

void setup() {
  
  GnssInit();

}

void loop() {

  if (xSemaphoreTake(GPS.mutex, (TickType_t)10) == pdTRUE) {
    if (GPS.val) {
        Serial.printf("LAT: %.6f, LON: %.6f, ALT: %.2f m\n", GPS.lat, GPS.lon, GPS.alt);
    }
    xSemaphoreGive(GPS.mutex);
  }
  delay(1000);

}
