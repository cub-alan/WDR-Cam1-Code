//Jacob Holwill 10859926
// a header file used to initialise gnss functions and the structure for gnss data

// ensure the .hpp file doesnt get defined more then once
#ifndef GPS_HPP
#define GPS_HPP

// include all nessesary librarys for the gnss
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_http_server.h"
#include <HardwareSerial.h>
#include <string.h>
#include <stdlib.h>

// create a group of data used to store all relevent info for the gnss
struct GnssData {

  // positional variables
  double lat = 0.0;
  double lon = 0.0;
  float alt = 0.0;

  // num of satellites connected
  int satellites = 0;

  // time variables
  int hour = 0;
  int min = 0;
  int sec = 0;

  // checks to see if gnss has a fix/ receiving data
  bool Fix_Val = false;
  bool data_Received = false;

  // initialise a mutex used to protect and read gnss data from other tasks
  SemaphoreHandle_t mutex = NULL; 
};

// be able to utilise data from the struct using namespace GPS
extern GnssData GPS; 

// initialise gnss functions
void Gnss_init();
void GnssTask(void *param);
esp_err_t GPS_Status_Update(httpd_req_t *Status_Request);

#endif