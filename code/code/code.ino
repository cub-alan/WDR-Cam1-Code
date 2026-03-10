#include <WiFi.h>
#include "Gps.hpp"
#include "Cam.hpp"
#include "OTA.hpp"
#include "Telnet.hpp"

// set the ssid and password to create the web server on (using my phone hotspot)
const char* ssid = "iPhone"; 
const char* password = "12345678";

// --- STATIC IP FOR IPHONE HOTSPOT ---
//IPAddress local_IP(172, 20, 10, 50);  // The IP you want (e.g., .50)
//IPAddress gateway(172, 20, 10, 1);   // iPhone Hotspot gateway is usually .1
//IPAddress subnet(255, 255, 255, 240); // iPhone uses a specific subnet mask
//IPAddress primaryDNS(8, 8, 8, 8);    // Google DNS
void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

 // if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
  //  Serial.println("STA Failed to configure Static IP");
 // }

  Cam_init();
  Serial.println("CAM Initialized");
  delay(2000);

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  int attempts = 0;
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
  

  // 4. Staggered Boot (Wait 3 seconds for power to settle)
  //delay(3000);

  //Gnss_init();
  //Serial.println("GNSS Initialized");
  //delay(3000); // Wait again

  // 5. Start Camera (This is the high-power event)
  //Cam_init();
  //Serial.println("CAM Initialized");
  //delay(2000);

  // 6. Start Servers
  //OTA_Init();
  //Telnet_Init();
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