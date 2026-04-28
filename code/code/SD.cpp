#include "SD.hpp"
#include <WiFi.h>

static SemaphoreHandle_t SDMutex = NULL;
static bool SD_send = false;
static File root;
static File currentFile;

//String Detection_IP = "192.168.4.128"; // on Robot-Stu1
String Detection_IP = "172.20.10.2"; // on iPhone

void SD_Init() {
    delay(200);
    
    SD_MMC.setPins(7, 9, 8);// set the SD card pins CLK, CMD, D0


    if (!SD_MMC.begin("/sdcard", true)) { // if the SD card fails to start skip
        Serial.println("SD init failed");
        return;
    }

    delay(200);

    uint8_t cardType = SD_MMC.cardType();// read the SD card type 

    if (cardType == CARD_NONE) {// check if there is a card and if not skip
        Serial.println("No SD card attached");
        return;
    }

    SDMutex = xSemaphoreCreateMutex(); // create a mutx for the sd 

    xTaskCreatePinnedToCore(SD_Task,"SD Task",8192,NULL,2,NULL,1); // create a multithreading task pinned to core 1
}

void SD_Start_Sending() {
    if (xSemaphoreTake(SDMutex, portMAX_DELAY)) { // check the mutex is free
        // open the file and set the send variable to true
        root = SD_MMC.open("/");
        currentFile = root.openNextFile();
        SD_send = true;

        xSemaphoreGive(SDMutex); // give back the mutex
    }
}

void SD_Stop_Sending() {
    SD_send = false; // stop the code from sending data
}

bool Send_File(File file) {
    if (WiFi.status() != WL_CONNECTED) { // if wifi connection failes return false
        return false;
    }

    HTTPClient http;

    String serverURL = "http://"+ Detection_IP+":8000/api/upload_file"; // set the send location

    http.begin(serverURL);// connect to send location

    // Set headers
    http.addHeader("Content-Type", "application/octet-stream");
    http.addHeader("File-Name", String(file.name()));

    int fileSize = file.size();// read file size

    int httpResponseCode = http.sendRequest("POST", &file, fileSize); // send the file and file size 

    if (httpResponseCode > 0) {// if sent print debug code
        Serial.print("HTTP Response: ");
        Serial.println(httpResponseCode);

        http.end(); // end the http send

        if (httpResponseCode == 200) { // its sent return true
            return true;
        }
    } else { // if not write debug code 
        Serial.print("Upload failed: ");
        Serial.println(http.errorToString(httpResponseCode));
    }
    http.end();
    return false;
}

void SD_Task(void *param) {

    while (true) {

        if (!SD_send) { // if the SD sending is set as true continue
            vTaskDelay(pdMS_TO_TICKS(100)); 
            continue;
        }

        if (xSemaphoreTake(SDMutex, pdMS_TO_TICKS(50))){ // check if mutex is free 
            if (!currentFile) {// not on the current file
                root.close();// close the file
                SD_send = false;
                xSemaphoreGive(SDMutex); // give back the mutex
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            String filename = String(currentFile.name()); // get the file name
            if (!filename.startsWith("/")){ // if it doesnt start fith a \ add one to the start
                filename = "/" + filename;
            }
            bool ok = Send_File(currentFile); // check if file is sent ok

            currentFile.close(); // close file

            if (ok) { // if file sent
                SD_MMC.remove(filename); // remove the file
            } else { // if not print debug
                Serial.println("Upload failed: " + filename);
            }

            currentFile = root.openNextFile(); // open next file 

            xSemaphoreGive(SDMutex); // close the mutex
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

esp_err_t SD_List_Handler(httpd_req_t *req) {

    if (!xSemaphoreTake(SDMutex, pdMS_TO_TICKS(100))) {// if the mutex is busy return
        return httpd_resp_send_500(req);
    }

    // open the next file 
    File root = SD_MMC.open("/");
    File file = root.openNextFile();

    String json = "[";

    while (file) {
        if (json.length() > 1) json += ",";
        json += "\"" + String(file.name()) + "\"";
        file = root.openNextFile();
    }

    json += "]";

    xSemaphoreGive(SDMutex); // give back the mutex

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json.c_str(), json.length()); // return the name and length 
}


esp_err_t SD_File_Handler(httpd_req_t *req) {

    char query[128];
    char filepath[96];// create a file path vector

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "name", filepath, sizeof(filepath)) != ESP_OK) { // if there is an error with the file pah return
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_sendstr(req, "Missing file name");
    }

    if (filepath[0] != '/') { // if the file doesnt start with / add it to the start
        char fixed[96];
        snprintf(fixed, sizeof(fixed), "/%s", filepath);
        strncpy(filepath, fixed, sizeof(filepath));
    }

    if (!xSemaphoreTake(SDMutex, pdMS_TO_TICKS(100))) {  // if mutex is not free return
        return httpd_resp_send_500(req);
    }

    File file = SD_MMC.open(filepath); //open the file path

    if (!file) { // if there is no file
        xSemaphoreGive(SDMutex); // give back the muttex
        return httpd_resp_send_404(req);
    }

    httpd_resp_set_type(req, "application/octet-stream");

    char buffer[1024];// create a bufer
    size_t readBytes;// read size of file 

    while ((readBytes = file.readBytes(buffer, sizeof(buffer))) > 0) { // check the file is small enough to write into the buffer
        httpd_resp_send_chunk(req, buffer, readBytes);
    }

    file.close(); // close file
    xSemaphoreGive(SDMutex); // give back mutex

    httpd_resp_send_chunk(req, NULL, 0); // if file is small enough send
    return ESP_OK;
}


esp_err_t SD_Delete_Handler(httpd_req_t *req) {

    char filepath[64]; // create fle path buffer 
    if (httpd_req_get_url_query_str(req, filepath, sizeof(filepath)) != ESP_OK) { // check the filepath and if error return and show debug
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Bad Request", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    if (!xSemaphoreTake(SDMutex, pdMS_TO_TICKS(100))) { // check mutex is free
        return httpd_resp_send_500(req);
    }

    bool success = SD_MMC.remove(filepath); // if succesfl then remove file

    xSemaphoreGive(SDMutex); // give back mutex

    if (success) { // if success return that its been deleted
        return httpd_resp_sendstr(req, "Deleted");
    } else { // if not successful show error code
        return httpd_resp_send_500(req);
    }
}

bool SD_Busy() {
    return SD_send; // if funtion called return the SD send 
}

