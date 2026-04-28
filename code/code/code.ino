//Jacob Holwill 10859926
//this file is to intergrate my other code together to create the initialisation and loop for the camera 1 operation

#include "MyLib.hpp"
#include <ESPmDNS.h>

volatile SystemMode currentMode = MODE_STREAM; //  set the starting mode
volatile bool stream_active = true; 

#define Sample_Sync_Pin D1 // offline image sync between cams
volatile uint32_t sample_id = 0; // id to match data from same sample

unsigned long Prev_Sample = 0;

void setup() {

  Serial.begin(115200); // set the terminal baud rate and begin the terminal
  Serial.setDebugOutput(true);
  
  for(int i=0;i<40;i++){
    Serial.println(); // clear 40 lines of the terminal 
  } 

  Gnss_init(); // initialise the gnss
  Serial.println("GNSS Initialized");
  delay(2000); // Wait 2000 ms

  SD_Init(); // initialise the SD card 
  Serial.println("SD Initialized");

  Cam1_init(); // initilise the Camera
  Serial.println("CAM Initialized");
  delay(2000); // Wait 2000 ms

  Light_init(); // initialise the ring light and LDR code
  Serial.println("Lighting Initialized");

  pinMode(Sample_Sync_Pin, OUTPUT);
  digitalWrite(Sample_Sync_Pin, LOW);

  WIFI_Connect(); // attempt to connect to the wifi
  
  if (WiFi.status() == WL_CONNECTED){ // if wifi is connected
    Cam1_Server_Init();

    // print out the weblinks for the Camera and gnss
    Serial.print("for camera use 'http://");
    Serial.print(WiFi.localIP()); // paste the ip of the camera stream 
    Serial.println("/stream1");
  }
}

// creates a function to sample data from both esps when in sd mode
void Trigger_Sample() {

  sample_id++; // increase the id of the by 1 to keep relevent data grouped

  digitalWrite(Sample_Sync_Pin, HIGH); // send a pulse to the other board
  delayMicroseconds(200);
  digitalWrite(Sample_Sync_Pin, LOW); // stop the pulse

  camera_fb_t *fb = esp_camera_fb_get(); // get the current matrix frame

  if (fb) { // if the frame is received 

    String filename = "/cam1_" + String(sample_id) + ".jpg"; //name the cam 1 file of this id group

    File file = SD_MMC.open(filename, FILE_WRITE); // create the file in writeable format
    if (file) { // if the file has been created
      file.write(fb->buf, fb->len); // print the frame into the file
      file.close(); // close the file
    }
    esp_camera_fb_return(fb); // give back the matrix
  }

  if (xSemaphoreTake(GPS.mutex, pdMS_TO_TICKS(50))) { // if the mutex is free

    String filename = "/gnss_" + String(sample_id) + ".json"; // name the gnss file with this groups id

    File file = SD_MMC.open(filename, FILE_WRITE); // create the file in writeable format
    if (file) { // check if the file is created 
      // print the gnss data into the file
      file.printf( "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f,\"sats\":%d,\"h\":%d,\"m\":%d,\"s\":%d}",GPS.lat, GPS.lon, GPS.alt,GPS.satellites,GPS.hour, GPS.min, GPS.sec);
      file.close(); // close the file
    }
    xSemaphoreGive(GPS.mutex); // return the mutex for the gnss
  }
}

void loop()
{
  static bool server_running = (WiFi.status() == WL_CONNECTED); // retreive the bool value for wifi connection status
  static bool sd_active = false;
  static unsigned long WIFI_Retry = 0; // create a variable to be able to do non blocking timers

  Light_Check(); // read the ldr vlue and if below the threshold turn on the ring light
  vTaskDelay(pdMS_TO_TICKS(10));

  if (currentMode == MODE_STREAM) { // if in streaming mode
      SD_Stop_Sending(); // dont send SD card 
      sd_active = false;
  }

  else if (currentMode == MODE_SD) { // if in SD mode
    stream_active = false; // turn off the stream
    if (!sd_active && WiFi.status() == WL_CONNECTED) { // if SD is availabe
        SD_Start_Sending();   // upload/retrieve only when webserver/WiFi exists
        sd_active = true;
    }

    if (millis() - Prev_Sample > 2000) { // every 2 seconds
        Prev_Sample = millis();
        Trigger_Sample();     // still samples to SD in SD mode
    }
}

  vTaskDelay(pdMS_TO_TICKS(10));

  if (WiFi.status() == WL_DISCONNECTED && millis() - WIFI_Retry > 5000) { // if the wifi disconects for longer then five seconds
    WIFI_Retry = millis(); // set the retry to the millis for future measurements 
    WiFi.disconnect(true); // disconect the wifi
    WiFi.begin(ssid, password); // start the wifi
  }

  vTaskDelay(pdMS_TO_TICKS(10));
}