#include "Gps.hpp"
#include <HardwareSerial.h>

HardwareSerial GNSS(1); // sets the gnss serial to uart 1

GnssData GPS; // get the global structure for the gnss data

void Gnss_init(){
  GPS.mutex = xSemaphoreCreateMutex(); // initialise the mutex for the structure
  GNSS.begin(9600,SERIAL_8N1,44,43); // set uart 1 baud rate to 9600 with pins 44 (TX) and 43 (RX)
  xTaskCreatePinnedToCore(GnssTask,"GNSS Task",4096,NULL,1,NULL,0); // create a gnss task on core 0 (task function, name of task, stack size, parameter passed in, priority of the task, pass back a handle, core number )
}

bool Check (const char* s){
  if (s[0] != '$'){ // all gnss data starts with $ so this checks its is reading correctly 
    return false; // if it doesnt start with $ its not valid
  }
  uint8_t CheckSum = 0;
  const char* p = s+1; // skips over the $ at the start 

  // xor all characters till the end of the string
  while (*p && *p != '*'){
    CheckSum ^= *p++;
  }
  if (*p == '*'){ // if the end is reached compare the calculated checksum with the received checksum
    long receive = strtol(p+1,NULL,16); // convert the string to a long int
    return CheckSum == receive;
  }
  return false; //if the end is not reached return false
}

void GnssTask(void *GnssParameters){
  char buff[128];
  int idx = 0;
  while (true){
    while (GNSS.available()){ // wait until the gnss is veiwable
      char c = GNSS.read();
      if (c == '\n'||c=='\r'){
        if (idx > 0){
          buff[idx] ='\0';

          if (strstr(buff,"$GPGGA")|| strstr(buff,"$GNGGA")&&Check(buff)){
            char* GPSfield[15];
            int fieldIDX = 0;
            char* token = buff;

            while (token && fieldIDX < 15){
              GPSfield[fieldIDX++] = token;
              token = strchr(token,',');
              if (token) *token++ = '\0';
            }
            if (fieldIDX>7){
              const char* StrLat = GPSfield[2];
              const char* DirLat = GPSfield[3];
              const char* StrLon = GPSfield[4];
              const char* DirLon = GPSfield[5];
              const char* StrAlt = GPSfield[9];

              int GPSquality = atoi(GPSfield[6]);

              if (GPSquality > 0 && strlen(StrLat) > 0 && strlen(StrLon) > 0){
                double PureLat = atof(StrLat);
                int DegLat = (int)(PureLat / 100);
                double Latitude = DegLat +(PureLat - (DegLat*100))/60.0;
                if (DirLat[0] == 'S'){
                  Latitude = -Latitude;
                } 
                double PureLon = atof(StrLon);
                int DegLon = (int)(PureLon / 100);
                double Longitude = DegLon +(PureLon - (DegLon*100))/60.0;
                if (DirLon[0] == 'W'){
                  Longitude = -Longitude;
                } 
                float Altitude = atof(StrAlt);

                if (xSemaphoreTake(GPS.mutex,portMAX_DELAY)){
                  GPS.lat = Latitude;
                  GPS.lon = Longitude;
                  GPS.alt = Altitude;
                  GPS.val = true;
                  xSemaphoreGive(GPS.mutex);
                }
              }
            }
          }
         idx = 0;
        }
      } 
      else if(idx< sizeof(buff)-1){
        buff[idx++] = c;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // slight delay to allow for over tasks to run
  }
}