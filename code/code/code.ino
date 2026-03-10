#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Gps.hpp"
#include "Cam.hpp"
#include "OTA.hpp"
#include "Telnet.hpp"

const char* ssid = "iPhone";
const char* password = "12345678";

// --- STATIC IP FOR IPHONE HOTSPOT ---
IPAddress local_IP(172, 20, 10, 50);  // The IP you want (e.g., .50)
IPAddress gateway(172, 20, 10, 1);   // iPhone Hotspot gateway is usually .1
IPAddress subnet(255, 255, 255, 240); // iPhone uses a specific subnet mask
IPAddress primaryDNS(8, 8, 8, 8);    // Google DNS

void setup() {
  // 1. Minimum Power State
  setCpuFrequencyMhz(80);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  // 2. Configure Static IP
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("STA Failed to configure Static IP");
  }

  // 3. Start WiFi only (lowest power draw)
  WiFi.begin(ssid, password);
  WiFi.setSleep(true);
  WiFi.setTxPower(WIFI_POWER_8_5dBm); 

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  // 4. Staggered Boot (Wait 3 seconds for power to settle)
  delay(3000);

  Gnss_init();
  Serial.println("GNSS Initialized");
  delay(3000); // Wait again

  // 5. Start Camera (This is the high-power event)
  Cam_init();
  Serial.println("CAM Initialized");
  delay(2000);

  // 6. Start Servers
  OTA_Init();
  Telnet_Init();
  Server_init();

  Serial.println("\n--- SYSTEM READY ---");
  Serial.print("Access at: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
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
}