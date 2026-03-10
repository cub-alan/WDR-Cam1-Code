#include <WiFi.h>
#include "Gps.hpp"
#include "Cam.hpp"

// set the ssid and password to create the web server on (using my phone hotspot)
const char* ssid = "iPhone"; 
const char* password = "12345678";

void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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
  
  Gnss_init();
  Serial.println("GNSS Initialized");
  delay(2000); // Wait again

}

void loop() {
  /*
  OTA_Handle();
  Telnet_Handle(); // Added from the previous Telnet improvement

  static unsigned long lastUpdate = 0;
  static int DotCount = 0;

  // Use non-blocking timing instead of delay(1000)
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();

    if (xSemaphoreTake(GPS.mutex, (TickType_t)10) == pdTRUE) {
      if (GPS.val) {
          Wifi_Terminal_Write("\033[3;1H LAT: %.6f, LON: %.6f, ALT: %.2f m\n", GPS.lat, GPS.lon, GPS.alt);
      }
      else {
        Wifi_Terminal_Write("\033[3;1H GPS STATUS: Connecting");
        for(int i=0; i<DotCount; i++) Wifi_Terminal_Write(".");
        Wifi_Terminal_Write("\033[K");
        DotCount = (DotCount + 1) % 4;
      }
      xSemaphoreGive(GPS.mutex);
    }
  }
  */
}