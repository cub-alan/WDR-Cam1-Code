//Jacob Holwill 10859926
//this file is to intergrate my other code together to create the initialisation and loop for the camera 1 operation

#include "MyLib.hpp"

// create the two modes the sytem can be in if wifi is connected or not
enum SystemMode {
  MODE_WIFI,
  MODE_SD
};
SystemMode currentMode = MODE_SD; // sets the defualt mode to SD as wifi is not yet connected

#define Sample_Sync_Pin D1 // offline image sync between cams
volitile uint32_t sample_id = 0; // id to match data from same sample

unsigned long Prev_Sample = 0;
const unsigned long Sample = 2000;

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

  pinMode(Sample_Sync_Pin, OUTPUT);
  digitalWrite(Sample_Sync_Pin, LOW);

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

// creates a function to sample data from both esps when in sd mode
void Trigger_Sample() {

  sample_id++; // increase the id of the by 1 to keep relevent data grouped

  digitalWrite(SYNC_PIN, HIGH); // send a pulse to the other board
  delayMicroseconds(200);
  digitalWrite(SYNC_PIN, LOW); // stop the pulse

  camera_fb_t *fb = esp_camera_fb_get(); // get the current matrix frame
  if (fb) { // if the frame is received 

    String filename = "/cam1_" + String(sample_id) + ".jpg"; //name the cam 1 file of this id group

    File file = SD.open(filename, FILE_WRITE); // create the file in writeable format
    if (file) { // if the file has been created
      file.write(fb->buf, fb->len); // print the frame into the file
      file.close(); // close the file
    }
    esp_camera_fb_return(fb); // give back the matrix
  }

  if (xSemaphoreTake(GPS.mutex, pdMS_TO_TICKS(50))) { // if the mutex is free

    String filename = "/gnss_" + String(sample_id) + ".json"; // name the gnss file with this groups id

    File file = SD.open(filename, FILE_WRITE); // create the file in writeable format
    if (file) { // check if the file is created 
      // print the gnss data into the file
      file.printf( "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f,\"sats\":%d,\"h\":%d,\"m\":%d,\"s\":%d}",
      GPS.lat, GPS.lon, GPS.alt,GPS.satellites,GPS.hour, GPS.min, GPS.sec);
      file.close(); // close the file
    }
    xSemaphoreGive(GPS.mutex); // return the mutex for the gnss
  }
}

void loop()
{
  static bool server_running = (WiFi.status() == WL_CONNECTED); // retreive the bool value for wifi connection status
  static unsigned long WIFI_Retry = 0; // create a variable to be able to do non blocking timers

  // sets the mode to wifi if not already in wifi mode
  if (WiFi.status() == WL_CONNECTED){
    if (currentMode != MODE_WIFI){
      currentMode = MODE_WIFI;
      Serial.println("Switched to WIFI mode");

      Cam1_Server_Init(); // start the server for the camera
      server_running = true; // set the server activity check to true

      SD_Start_Sending(); // start snding any data on the sd card across wifi

      // print out the weblinks for the Camera and gnss
      Serial.print("for camera use 'http://");
      Serial.print(WiFi.localIP()); // paste the ip of the camera stream 
      Serial.println("/stream1");

      Serial.print("for location stats use 'http://");
      Serial.print(WiFi.localIP()); // paste the ip of the gnss stream 
      Serial.println("/status");
    }
  } 
  // switches to sd mode if wifi disconected and not alreay in sd mode
  else{
    if (currentMode != MODE_SD){
      currentMode = MODE_SD;
      Serial.println("Switched to SD mode");

      SD_Stop_Sending(); // stop attempting to send data over wifi

      if (server_running) { // if the server is active
      httpd_stop(Server); // deactivate the server
      server_running = false; // set the checking int to false
      }
    }
  }

  if (WiFi.status() == WL_DISCONNECTED && millis() - WIFI_Retry > 5000) { // if the wifi disconects for longer then five seconds
    WIFI_Retry = millis(); // set the retry to the millis for future measurements 
    WiFi.disconnect(true); // disconect the wifi
    WiFi.begin(ssid, password); // start the wifi
  }

  if (currentMode == MODE_SD) { // if in sd card mode
    if (millis() - Prev_Sample > 2000) { // every 2 seconds
      Prev_Sample = millis(); // set the variable to the current time to allow for more sampling to be done
      Trigger_Sample(); // execute the sampling function
    }
  }

  Light_Check(); // read the ldr vlue and if below the threshold turn on the ring light
  vTaskDelay(pdMS_TO_TICKS(10));
}