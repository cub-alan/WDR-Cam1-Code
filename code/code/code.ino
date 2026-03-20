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

  WIFI_Connect(); // connect to the wifi
  
  Cam1_Server_Init(); // connect the gnss data stream and camera data stream to the wifi

  Serial.print("for cam use 'http://");
  Serial.print(WiFi.localIP()); // paste the ip of the stream so it can be opened on the browser
  Serial.println("/stream");

  Serial.print("for gps use 'http://");
  Serial.print(WiFi.localIP()); // paste the ip of the stream so it can be opened on the browser
  Serial.println("/status");
}

void loop()
{
  static unsigned long timer = 0; // create a variable to be able to do non blocking timers

  if (millis() - timer > 1000){ // every 1000ms 
    timer = millis(); // set the millis to the timer to allow for the next loop to run at 1000ms 

    if (xSemaphoreTake(GPS.mutex, pdMS_TO_TICKS(20))){ // if the mutex is readable
      
      // retreaive the data from the mutex
      bool fix_check = GPS.Fix_Val;
      bool data_check = GPS.data_Received;

      double lat = GPS.lat;
      double lon = GPS.lon;
      int sats = GPS.satellites;

      if (data_check){ //if data has been checked
        GPS.data_Received = false; // set the data received to false
      }
      xSemaphoreGive(GPS.mutex); // lock the mutex

      if (fix_check){ // if there is a gnss lock then print the data
        Serial.printf("\n| GPS FIX | LAT: %.6f LON: %.6f SATS: %d\n", lat, lon, sats);
      }
      else if (GPS.data_Received ||data_check){ // if its receiving lines but not yet got a lock print waiting for fix line
        Serial.println("\n| GPS COMMUNICATING | Waiting for satellite fix...");
      }
      else{ // if its not receiving anything show that there is an error
        Serial.println("\n| GPS NO DATA | error not receiving");
      }
    }
  }
}