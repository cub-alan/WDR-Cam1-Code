//Jacob Holwill 10859926
//this file is to intergrate my other code together to create the initialisation and loop of my code

#include "MyLib.hpp"

void setup() {

  Serial.begin(115200); // set the terminal baud rate and begin the terminal
  Serial.setDebugOutput(true);
  
  for(int i=0;i<40;i++){
    Serial.println(); // clear 40 lines of the terminal 
  } 

  Gnss_init(); // initialise the gnss
  Serial.println("GNSS Initialized");
  delay(2000); // Wait 2000 ms

  Cam1_init(); // initilise the Camera
  Serial.println("CAM Initialized");
  delay(2000); // Wait 2000 ms

  SD_Init();

  WIFI_Connect(); // connect to the wifi
  
  if (WiFi.status() == WL_CONNECTED){
    Cam1_Server_Init(); // connect the gnss data stream and camera data stream to the wifi
  }

  Serial.print("for camera use 'http://");
  Serial.print(WiFi.localIP()); // paste the ip of the stream so it can be opened on the browser
  Serial.println("/stream");

  Serial.print("for location stats use 'http://");
  Serial.print(WiFi.localIP()); // paste the ip of the stream so it can be opened on the browser
  Serial.println("/status");
}

void loop()
{
  static bool server_running = (WiFi.status() == WL_CONNECTED);

  static unsigned long timer = 0, WIFI_Retry = 0; // create a variable to be able to do non blocking timers

  static double Prev_Lat = 0, Prev_Lon = 0;

  const int SD_SAMPLE_RATE = 2000;

  bool WIFI_Connection_Check = (WiFi.status() == WL_CONNECTED);

  if (!WIFI_Connection_Check && millis() - WIFI_Retry > 2000) { // retry wifi connect every 2 seconds
    WIFI_Retry = millis();
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }
  if (WIFI_Connection_Check && !server_running) {
    Cam1_Server_Init();
    server_running = true;
    Serial.println("Switched to WiFi streaming mode");
  }

  if (!WIFI_Connection_Check && !server_running){
    if (millis() - timer > SD_SAMPLE_RATE){ // every 1000ms 
      timer = millis(); // set the millis to the timer to allow for the next loop to run at 1000ms 

      GnssData snapshot;

      if (xSemaphoreTake(GPS.mutex, pdMS_TO_TICKS(20))){ // if the mutex is readable
        snapshot = GPS;
        xSemaphoreGive(GPS.mutex);
      } 

      camera_fb_t *fb = esp_camera_fb_get();

      SD_Sample(fb, snapshot);

      esp_camera_fb_return(fb);

      Prev_Lat  = snapshot.lat;
      Prev_Lon  = snapshot.lon;
      
    }
  }
}