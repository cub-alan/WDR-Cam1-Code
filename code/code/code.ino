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

  static unsigned long GnssUpdate = 0;

  //nonblocking delay(1000)
  if (millis() - GnssUpdate > 1000) {
    GnssUpdate = millis();

    if (xSemaphoreTake(GPS.mutex, (TickType_t)10) == pdTRUE) {
      if (GPS.val) {
          Serial.printf("| GPS: LAT: %.6f, LON: %.6f\", GPS.lat, GPS.lon");
      } else {
          Serial.print("| GPS: Searching...\n ");
      }
      xSemaphoreGive(GPS.mutex);
    }
  } 
}