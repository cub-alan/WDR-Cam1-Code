//Jacob Holwill 10859926
// the point of this file is to hold the gnss functions for reading and parsing the gnss data and streaming the gnss data 

#include "Gps.hpp"

HardwareSerial GNSS(1);
GnssData GPS; // retreive the GnssData struct and give in namespace GPS

//define nessesary pins and baud rate
#define GNSS_RX 43
#define GNSS_TX 44
#define GNSS_BAUD 9600

//========================================================================================================================================================

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
//============================================================================================================================================================

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
//=====================================================================================================================================================

void GnssTask(void *param){
  char line[128];// creates a buffer to store the gnss info line in
  int Current_Point = 0; // creates a variable to hold pointer posision 

  while (true){

    while (GNSS.available()){ // check if the gnss is readable

      char GNSS_char = GNSS.read(); // create a variable to read the current character in the gnss line

      if (GNSS_char == '\r') { // if its a command to move to the start of terminal
        continue; // ignore it and move on
      }

      if (GNSS_char == '\n'){ // if the command is to create a new line
        line[Current_Point] = '\0'; // add a null terminator to make the c string valid
        Current_Point = 0; // reset the buffer 

        if (strlen(line) < 10){ //if the length of teh string is less the 10 characters
          continue; // ignore it and move on
        }

        if (!Checksum(line)){ // sends the line through the checksum function 
          continue; // if checksum is false ignore the line and move on
        }

        if (strncmp(line, "$GNGGA", 6) != 0 && strncmp(line, "$GPGGA", 6) != 0){ // checks for the lines with the info i want to receive
          continue; // if it is a line with the needed info continue the function
        }
        
        //create an array to store each bit of data that is seperated by commas
        char *fields[15];
        int field = 0;

        char *Line_Split = strtok(line, ",");//splits the line by the commas

        while (Line_Split && field < 15){ // while the are area of the split line left and the feild is less then 15
          fields[field++] = Line_Split; // take the feild and position and put that section of the line split into the feilds
          Line_Split = strtok(NULL, ","); // goes to the next section of the split line
        }

        if (field < 10) {// ensure there is the right amount of feilds
          continue; // continue if there are less then 10 feilds
        }

        int quality = atoi(fields[6]); // set feild 6 to a quality check

        if (quality == 0){ // if there is no gps signal quality
          if (xSemaphoreTake(GPS.mutex, portMAX_DELAY)){ // if the mutex is accessable
            GPS.Fix_Val = false;
            xSemaphoreGive(GPS.mutex); // lock mutex
          }
          continue; // continue the function
        }

        //set the feilds to their own integers
        const char* latStr = fields[2];
        const char* latDir = fields[3];
        const char* lonStr = fields[4];
        const char* lonDir = fields[5];
        const char* altStr = fields[9];
        const char* satStr = fields[7];
        const char* timeStr = fields[1];

        if (!latStr || !lonStr || !latDir || !lonDir || !timeStr) {
          continue;
        }

        double Convert_Lat = atof(latStr);
        double Convert_Lon = atof(lonStr);

        int latDeg = Convert_Lat / 100;
        int lonDeg = Convert_Lon / 100;

        double lat = latDeg + (Convert_Lat - latDeg * 100) / 60.0;
        double lon = lonDeg + (Convert_Lon - lonDeg * 100) / 60.0;

        if (latDir[0] == 'S') {
          lat = -lat;
        } 
        if (lonDir[0] == 'W') {
          lon = -lon;
        } 

        float alt = atof(altStr);
        int satellites = atoi(satStr);

        int hour = 0;
        int min = 0;
        int sec = 0;

        if (timeStr && strlen(timeStr) >= 6){
          hour = (timeStr[0]-'0')*10 + (timeStr[1]-'0');
          min = (timeStr[2]-'0')*10 + (timeStr[3]-'0');
          sec = (timeStr[4]-'0')*10 + (timeStr[5]-'0');
        }
        if (xSemaphoreTake(GPS.mutex, portMAX_DELAY)){
          GPS.lat = lat;
          GPS.lon = lon;
          GPS.alt = alt;
          GPS.satellites = satellites;
          GPS.hour = hour;
          GPS.min = min;
          GPS.sec = sec;
          GPS.Fix_Val  = true;
          GPS.data_Received = true;
          xSemaphoreGive(GPS.mutex);
        }
      }
      else{
        if (Current_Point < sizeof(line) - 1){
          line[Current_Point++] = GNSS_char;
        }
        else{
          Current_Point = 0;
          continue;
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
//===============================================================================================================================================================

esp_err_t GPS_Status_Update(httpd_req_t *Status_Request) { // a static function that returns error for the status request
    char GNSS_Write[256]; // create a buffer for the GNSS to be able to write in
    if (xSemaphoreTake(GPS.mutex, 10) == pdTRUE) { // attempt to lock the mutex to be able to acess it and protect the GNSS data 

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
