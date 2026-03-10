#ifndef GPS_HPP
#define GPS_HPP

#include "Arduino.h"
#include "freertos/FreeRTOS.h" // library to allow for multithreading 
#include "freertos/semphr.h" // library to allow for mutex use 

struct GnssData { // create a structure to hold all of the gnss data
  float lat = 0.0; // create a variable for latitude
  float lon = 0.0; // create a variable for longatude
  float alt = 0.0; // create a variable for num of satilitse connected to
  int hour = 0;
  int minute = 0;
  int second = 0;
  bool val = false; // create a validation variable
  SemaphoreHandle_t mutex; // create a mutex for the structure
};

extern GnssData GPS; // create a global structure for the gnss data

// initialise all gps.cpp functions
void Gnss_init();
void GnssTask(void *GnssParameters);

#endif