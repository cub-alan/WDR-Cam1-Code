#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "Cam.hpp"
#include "Gps.hpp"

extern bool SD_Check;

void SD_Init();
bool SD_Sample(camera_fb_t *fb, GnssData data);

#endif