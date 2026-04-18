//Jacob Holwill 10859926
//this file is to intergrate my other code together to create the initialisation and loop for the camera 1 operation

#include "MyLib.hpp"

// create the two modes the sytem can be in if wifi is connected or not
enum SystemMode {
  MODE_WIFI,
  MODE_SD
};

SystemMode currentMode = MODE_SD; // sets the defualt mode to SD as wifi is not yet connected

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

  Light_init(); // initialise the ring light and LDR code
  Serial.println("Lighting Initialized");

  SD_Init(); // initialise the SD card 
  Serial.println("SD Initialized");

  WIFI_Connect(); // attempt to connect to the wifi
  
  if (WiFi.status() == WL_CONNECTED){ // if wifi is connected
    Cam1_Server_Init(); // initialise the server for camera 1
    currentMode = MODE_WIFI; // set the mode to WIFI

    // print out the weblinks for the Camera and gnss
    Serial.print("for camera use 'http://");
    Serial.print(WiFi.localIP()); // paste the ip of the camera stream 
    Serial.println("/stream1");

    Serial.print("for location stats use 'http://");
    Serial.print(WiFi.localIP()); // paste the ip of the gnss stream
    Serial.println("/status");
  }
}

void loop()
{
  static bool server_running = (WiFi.status() == WL_CONNECTED); // retreive the bool value for wifi connection status
  static unsigned long timer = 0, WIFI_Retry = 0; // create a variable to be able to do non blocking timers
  const int SD_SAMPLE_RATE = 2000; // creates a sample rate to store the images onto the SD card

  Light_Check(); // read the ldr vlue and if below the threshold turn on the ring light

  wl_status_t status = WiFi.status(); // check the wifi connection

  if (status == WL_CONNECTED && currentMode != MODE_WIFI) { // if wifi is connected and wasnt in wifi mode
    Serial.println("Switching to WIFI mode");
    
    Cam1_Server_Init(); // start the server for the camera
    server_running = true; // set the server activity check to true

    SD_Upload_Begin(); // start uploading sd card data

    // print out the weblinks for the Camera and gnss
    Serial.print("for camera use 'http://");
    Serial.print(WiFi.localIP()); // paste the ip of the camera stream 
    Serial.println("/stream1");

    Serial.print("for location stats use 'http://");
    Serial.print(WiFi.localIP()); // paste the ip of the gnss stream 
    Serial.println("/status");

    currentMode = MODE_WIFI; // set the mode to WIFI
  }

  if (status != WL_CONNECTED && currentMode != MODE_SD) { // if wifi is not connected
    Serial.println("Switching to SD mode");

    if (server_running) { // if the server is active
      httpd_stop(Server); // deactivate the server
      server_running = false; // set the checking int to false
    }
    currentMode = MODE_SD; // set to SD mode
  }

  if (WiFi.status() == WL_DISCONNECTED && millis() - WIFI_Retry > 5000) { // if the wifi disconects for longer then five seconds
    WIFI_Retry = millis(); // set the retry to the millis for future measurements 
    WiFi.disconnect(true); // disconect the wifi
    delay(100);
    WiFi.begin(ssid, password); // start the wifi
  }

  if (currentMode == MODE_SD) { // if in sd card mode
    if (millis() - timer > SD_SAMPLE_RATE){ // every time the sapleing rate is passed
      timer = millis(); // set the new timer to be able to continue sampling

      GnssData snapshot; // create a snapshot of gnss info
      if (xSemaphoreTake(GPS.mutex, pdMS_TO_TICKS(20))){ // acess the mutex for gnss data
        snapshot = GPS; // get the gnss snapshot
        xSemaphoreGive(GPS.mutex); // return the mutex
      }

      camera_fb_t *fb = esp_camera_fb_get(); // get the camera driver to capture a frame
      if (fb) { // if the cature succedes
        SD_Sample(fb, snapshot); // feed in the screan shot and gnss data into SD_Sample function
        esp_camera_fb_return(fb); // returns the camera buffer
      }
    }
  }
   if (currentMode == MODE_WIFI) { // if the mode is wifi 
        SD_Upload_Task(); // upload sd card data 
    }
}