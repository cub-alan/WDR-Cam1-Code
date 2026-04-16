//Jacob Holwill 10859926
//this file is to intergrate my other code together to create the initialisation and loop of my code

#include "MyLib.hpp"

enum SystemMode {
  MODE_WIFI,
  MODE_SD
};

SystemMode currentMode = MODE_SD;

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

  Lighting_Init();
  Serial.println("Lighting Initialized");

  SD_Init();
  Serial.println("SD Initialized");

  WIFI_Connect(); // connect to the wifi
  
  if (WiFi.status() == WL_CONNECTED){
    Cam1_Server_Init(); // connect the gnss data stream and camera data stream to the wifi
    currentMode = MODE_WIFI;

    Serial.print("for camera use 'http://");
    Serial.print(WiFi.localIP()); // paste the ip of the stream so it can be opened on the browser
    Serial.println("/stream1");

    Serial.print("for location stats use 'http://");
    Serial.print(WiFi.localIP()); // paste the ip of the stream so it can be opened on the browser
    Serial.println("/status");
  }
}

void loop()
{
  static bool server_running = (WiFi.status() == WL_CONNECTED);
  static unsigned long timer = 0, WIFI_Retry = 0; // create a variable to be able to do non blocking timers
  const int SD_SAMPLE_RATE = 2000;

  Lighting_Update();

  wl_status_t status = WiFi.status();

  if (status == WL_CONNECTED && currentMode != MODE_WIFI) {
    Serial.println("Switching to WIFI mode");
    
    Cam1_Server_Init();
    server_running = true;

    SD_Upload_Begin(); // start uploading sd card data

    Serial.print("for camera use 'http://");
    Serial.print(WiFi.localIP()); // paste the ip of the stream so it can be opened on the browser
    Serial.println("/stream1");

    Serial.print("for location stats use 'http://");
    Serial.print(WiFi.localIP()); // paste the ip of the stream so it can be opened on the browser
    Serial.println("/status");

    currentMode = MODE_WIFI;
  }

  if (status != WL_CONNECTED && currentMode != MODE_SD) {
    Serial.println("Switching to SD mode");

    if (server_running) {
      httpd_stop(Server);
      server_running = false;
    }
    currentMode = MODE_SD;
  }

  if (WiFi.status() == WL_DISCONNECTED && millis() - WIFI_Retry > 5000) {
    WIFI_Retry = millis();
    WiFi.disconnect(true);
    delay(100);
    WiFi.begin(ssid, password);
  }

  if (currentMode == MODE_SD) {
    if (millis() - timer > SD_SAMPLE_RATE){
      timer = millis();

      GnssData snapshot;
      if (xSemaphoreTake(GPS.mutex, pdMS_TO_TICKS(20))){
        snapshot = GPS;
        xSemaphoreGive(GPS.mutex);
      }

      camera_fb_t *fb = esp_camera_fb_get();
      if (fb) {
        SD_Sample(fb, snapshot);
        esp_camera_fb_return(fb);
      }
    }
  }
   if (currentMode == MODE_WIFI) {
        SD_Upload_Task();
    }
}