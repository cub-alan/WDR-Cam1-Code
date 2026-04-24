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
    
    SD_MMC.setPins(7, 9, 8); // CLK, CMD, D0

    if (!SD_MMC.begin("/sdcard", true)) { // if the SD card fails to start skip
        Serial.println("SD init failed");
        return;
    }

    delay(200);

    uint8_t cardType = SD_MMC.cardType();

    if (cardType == CARD_NONE) {
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

    String serverURL = "http://"+ Detection_IP+":8000/api/upload_file";

    http.begin(serverURL);

    // Set headers
    http.addHeader("Content-Type", "application/octet-stream");
    http.addHeader("File-Name", String(file.name()));

    int fileSize = file.size();

    int httpResponseCode = http.sendRequest("POST", &file, fileSize);

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response: ");
        Serial.println(httpResponseCode);

        http.end();

        if (httpResponseCode == 200) {
            return true;
        }
    } else {
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
            if (!currentFile) {
                root.close();
                SD_send = false;
                xSemaphoreGive(SDMutex);
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            String filename = String(currentFile.name());
            if (!filename.startsWith("/")){ 
                filename = "/" + filename;
            }
            bool ok = Send_File(currentFile);

            currentFile.close();

            if (ok) {
                SD_MMC.remove(filename);
            } else {
                Serial.println("Upload failed: " + filename);
            }

            currentFile = root.openNextFile(); 

            xSemaphoreGive(SDMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

esp_err_t SD_List_Handler(httpd_req_t *req) {

    if (!xSemaphoreTake(SDMutex, pdMS_TO_TICKS(100))) {
        return httpd_resp_send_500(req);
    }

    File root = SD_MMC.open("/");
    File file = root.openNextFile();

    String json = "[";

    while (file) {
        if (json.length() > 1) json += ",";
        json += "\"" + String(file.name()) + "\"";
        file = root.openNextFile();
    }

    json += "]";

    xSemaphoreGive(SDMutex);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json.c_str(), json.length());
}


esp_err_t SD_File_Handler(httpd_req_t *req) {

    char filepath[64];
    if (httpd_req_get_url_query_str(req, filepath, sizeof(filepath)) != ESP_OK) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Bad Request", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    if (!xSemaphoreTake(SDMutex, pdMS_TO_TICKS(100))) {
        return httpd_resp_send_500(req);
    }

    File file = SD_MMC.open(filepath);

    if (!file) {
        xSemaphoreGive(SDMutex);
        return httpd_resp_send_404(req);
    }

    httpd_resp_set_type(req, "application/octet-stream");

    char buffer[1024];
    size_t readBytes;

    while ((readBytes = file.readBytes(buffer, sizeof(buffer))) > 0) {
        httpd_resp_send_chunk(req, buffer, readBytes);
    }

    file.close();
    xSemaphoreGive(SDMutex);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


esp_err_t SD_Delete_Handler(httpd_req_t *req) {

    char filepath[64];
    if (httpd_req_get_url_query_str(req, filepath, sizeof(filepath)) != ESP_OK) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Bad Request", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    if (!xSemaphoreTake(SDMutex, pdMS_TO_TICKS(100))) {
        return httpd_resp_send_500(req);
    }

    bool success = SD_MMC.remove(filepath);

    xSemaphoreGive(SDMutex);

    if (success) {
        return httpd_resp_sendstr(req, "Deleted");
    } else {
        return httpd_resp_send_500(req);
    }
}

bool SD_Busy() {
    return SD_send;
}

