#ifndef GPS_HPP
#define GPS_HPP

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

struct GnssData {

  float lat = 0.0;
  float lon = 0.0;
  float alt = 0.0;

  int satellites = 0;

  int hour = 0;
  int min = 0;
  int sec = 0;

  bool val = false;
  bool dataReceived = false;

  SemaphoreHandle_t mutex;
};

extern GnssData GPS;

void Gnss_init();
void GnssTask(void *param);

#endif