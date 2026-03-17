//Jacob Holwill 10859926
//

#include "MyLib.hpp"

void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  for(int i=0;i<40;i++){
    Serial.println(); // clear the terminal 
  } 

  Gnss_init();
  Serial.println("GNSS Initialized");
  delay(2000); // Wait 2000 ms

  Cam1_init();
  Serial.println("CAM Initialized");
  delay(2000);

  WIFI_Connect();
  
  Cam1_Server_Init();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
}

void loop()
{
  static unsigned long timer = 0;

  if (millis() - timer > 1000)
  {
    timer = millis();

    if (xSemaphoreTake(GPS.mutex, pdMS_TO_TICKS(20))){
      
      bool fix_check = GPS.Fix_Val;
      bool data_check = GPS.data_Received;

      double lat = GPS.lat;
      double lon = GPS.lon;
      int sats = GPS.satellites;

      if (data_check){
        GPS.data_Received = false;
      }
      xSemaphoreGive(GPS.mutex);

      if (fix_check)
      {
        Serial.printf("| GPS FIX | LAT: %.6f LON: %.6f SATS: %d\n", lat, lon, sats);
      }
      else if (GPS.data_Received ||data_check)
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