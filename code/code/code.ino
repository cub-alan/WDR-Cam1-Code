#include <WiFi.h>
#include "Gps.hpp"
#include "Cam.hpp"

// set the ssid and password to create the web server on (using my phone hotspot)
const char* ssid = "iPhone"; 
const char* password = "12345678";

void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  for(int i=0;i<40;i++) Serial.println(); // clear the terminal 

  Gnss_init();
  Serial.println("GNSS Initialized");
  delay(2000); // Wait 2000 ms

  Cam_init();
  Serial.println("CAM Initialized");
  delay(2000);

  Serial.println("");
  Serial.print("Connecting to WIFI");

  WiFi.setHostname("xiao-sense");
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.setAutoReconnect(true);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  Server_init();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

}


void loop()
{
  static unsigned long timer = 0;

  if (millis() - timer > 1000)
  {
    timer = millis();

    if (xSemaphoreTake(GPS.mutex, pdMS_TO_TICKS(20)))
    {
      bool fix_check = GPS.val;
      bool data_check = GPS.dataReceived;

      double lat = GPS.lat;
      double lon = GPS.lon;
      int sats = GPS.satellites;

      GPS.dataReceived = false;

      xSemaphoreGive(GPS.mutex);

      if (fix_check)
      {
        Serial.printf("| GPS FIX | LAT: %.6f LON: %.6f SATS: %d\n", lat, lon, sats);
      }
      else if (data_check)
      {
        Serial.println("| GPS COMMUNICATING | Waiting for satellite fix...");
      }
      else
      {
        Serial.println("| GPS NO DATA | Check wiring or GPS power.");
      }
    }
  }
}