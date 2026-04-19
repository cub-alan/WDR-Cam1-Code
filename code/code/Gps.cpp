//Jacob Holwill 10859926
// the point of this file is to hold the gnss functions for reading and parsing the gnss data and streaming the gnss data 

#include "Gps.hpp"

HardwareSerial GNSS(1);
GnssData GPS; // retreive the GnssData struct and give in namespace GPS

//define nessesary pins and baud rate
#define GNSS_RX 44
#define GNSS_TX 43
#define GNSS_BAUD 9600

bool Checksum(const char* GNSS_Data_Line){ // a function that will return true or false from the pointer to the gnss line fed in
  if (*GNSS_Data_Line != '$') { // if the gnss data line does not start with the dolar sign 
    return false; // feed back false to function
  }

  uint8_t sum_check = 0; // create an unsigned 8 bit integer to store the calculated sum
  GNSS_Data_Line++; // move to the next character after the dollar sign

  while (*GNSS_Data_Line && *GNSS_Data_Line != '*'){ // if it is not the end of the line and stops if it reaches the check sum marker
    sum_check ^= *GNSS_Data_Line++; //the sum checker is now equal to the XOR of the characters
  }

  if (*GNSS_Data_Line == '*'){ // if the end of the line is reached
    int GNSS_get = strtol(GNSS_Data_Line + 1, NULL, 16);// read the data line including the astrix in base 16
    return sum_check == GNSS_get; // if the calculated and transmited check sum are equal return true
  }

  return false; // else return false
}

void Gnss_init(){
  GPS.mutex = xSemaphoreCreateMutex(); // create the mutex for the gps

  if (GPS.mutex == NULL) { // if mutex failed to create
    Serial.println("\n GPS mutex fail"); 
    return; // exit the function
  }

  GNSS.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX, GNSS_TX); // begin the GNSS with the set pins and baud rate 
  
  TaskHandle_t GNSS_Handler = NULL; // declair the task required
  xTaskCreatePinnedToCore(GnssTask,"GNSS Task",4096,NULL,1,&GNSS_Handler,1); //create the gnss task on core 1 (seperate from the camera) with GNSS_Handler as the task
}

void GnssTask(void *param){
  // create needed variables
  char line[128];
  int idx = 0;
  unsigned long last_data_time = 0;

  while (true){

    while (GNSS.available()){ // while the gnss is readable

      char c = GNSS.read(); // read the value of the gnss

      if (c == '\r') { // wait for a line end to continue
        continue;
      }

      if (c == '\n'){ // if theres a new line
        line[idx] = '\0';
        idx = 0;

        if (strlen(line) < 10){ // check the line is invalid skip
          continue;
        }

        if (!Checksum(line)){ //if line fails skip
          continue;
        }

        if (strncmp(line, "$GNGGA", 6) != 0){ // if the line does not start with $GNGGA skip
          continue;
        }
        
        // create variables for parsing 
        char *fields[15];
        int field = 0;
        char *Pointer_GNSS = line;

        while (Pointer_GNSS && field < 15) { // parse the line
          fields[field++] = Pointer_GNSS; // set the postion in the feild vector to pointer_GNSS
          Pointer_GNSS = strchr(Pointer_GNSS, ','); // make it so the feilds are seperated by the commas
          if (Pointer_GNSS) { // checks the pointer is valid
            *Pointer_GNSS = '\0'; set the charicter is null
            Pointer_GNSS++; // move to the next character
          }
        }

        if (field < 10) {// if the number of feilds is less then 10 skip
          continue;
        }

        int quality = atoi(fields[6]); // set the quality value to feild 6

        
        if (xSemaphoreTake(GPS.mutex, portMAX_DELAY)){ // check if the mutex is free and attempt to access it
          GPS.data_Received = true; // Set data Received variable to true when a valid line is received
          xSemaphoreGive(GPS.mutex); // return the mutex
        }

        if (quality == 0){ // if these is no fix on any satilites
          if (xSemaphoreTake(GPS.mutex, portMAX_DELAY)){ //access the mutex
            GPS.Fix_Val = false; // set the variable that checks its receiving a correct value to 0
            xSemaphoreGive(GPS.mutex); // return the mutex
          }
          continue; // skip the rest of the code
        }

        // set the values of all data to the value in their feild position on the received line 
        const char* latStr = fields[2];
        const char* latDir = fields[3];
        const char* lonStr = fields[4];
        const char* lonDir = fields[5];
        const char* altStr = fields[9];
        const char* satStr = fields[7];
        const char* timeStr = fields[1];

        if (!latStr || !lonStr || !latDir || !lonDir || !timeStr) { // check if all data is received and if not skip
          continue;
        }

        // these two variables take the received ascii test and converts them to a float value
        double Convert_Lat = atof(latStr);
        double Convert_Lon = atof(lonStr);

        // convert the received latitude and longatude from degree minute to decimal degree
        int latDeg = Convert_Lat / 100; // gets the int from the degree
        int lonDeg = Convert_Lon / 100; // gets the int from the degree

        double lat = latDeg + (Convert_Lat - latDeg * 100) / 60.0; // gets the decimal from the minute
        double lon = lonDeg + (Convert_Lon - lonDeg * 100) / 60.0; // get the decimal from the minute

        if (latDir[0] == 'S') { // if the latitude direction specifies south flip the polarity of the latitude value
          lat = -lat;
        } 
        if (lonDir[0] == 'W') { // if the longatude direction specifies west flip the polarity of the longatude value
          lon = -lon;
        } 

        // convert the values for the altitude and number of satilites from ascii to the desired variable type
        float alt = atof(altStr);
        int satellites = atoi(satStr);

        // initialise variables to find the time
        int hour = 0;
        int min = 0;
        int sec = 0;

        if (timeStr && strlen(timeStr) >= 6){ // check the time feild exists and has a value that is large enought to be the time
          // convert each feild to be the time 
          hour = (timeStr[0]-'0')*10 + (timeStr[1]-'0');
          min = (timeStr[2]-'0')*10 + (timeStr[3]-'0');
          sec = (timeStr[4]-'0')*10 + (timeStr[5]-'0');
        }
        if (xSemaphoreTake(GPS.mutex, portMAX_DELAY)){ // if the gnss mutex is free
          // set all values to the received value
          GPS.lat = lat;
          GPS.lon = lon;
          GPS.alt = alt;
          GPS.satellites = satellites;
          GPS.hour = hour;
          GPS.min = min;
          GPS.sec = sec;
          GPS.Fix_Val  = true;
          GPS.data_Received = true;
          xSemaphoreGive(GPS.mutex); // return the mutex
        }
      }
      else { // if there is not a new line
        if (idx < sizeof(line) - 1) { //if there is space in the buffer
          line[idx++] = c; // store the character in position index then add one to it
        } else { // if no space
          idx = 0; // reset the index
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

esp_err_t GPS_Status_Update(httpd_req_t *Status_Request) { // a function that returns error for the status request
    char GNSS_Write[256]; // create a buffer for the GNSS to be able to write in
    if (xSemaphoreTake(GPS.mutex, 10) == pdTRUE) { // attempt to lock the mutex to be able to acess it and protect the GNSS data 

        const char* GNSS_Check = GPS.Fix_Val ? "true" : "false"; // ceate a check to see if the gnss is receiving anything

        snprintf(GNSS_Write, sizeof(GNSS_Write),  // print the GNSS_Write buffer witht the following text while keeping the buffer size
        "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f,\"sats\":%d,\"h\":%d,\"m\":%d,\"s\":%d,\"valid\":%s}", // write this to the buffer
        GPS.lat, GPS.lon, GPS.alt, GPS.satellites, GPS.hour, GPS.min, GPS.sec, GNSS_Check); // write the data to its corrosponding % in the previous text

        xSemaphoreGive(GPS.mutex); // unlock the mutex
    } 
    else { // if mutex is unable to be locked/read
        strcpy(GNSS_Write, "{\"error\":\"GPS mutex is busy\"}"); // if the mutex is being used create this error message in the buffer
    }
    httpd_resp_set_type(Status_Request, "application/json"); // tells the browser the respons of the json
    return httpd_resp_send(Status_Request, GNSS_Write, strlen(GNSS_Write)); // sends the json back to the client
}
