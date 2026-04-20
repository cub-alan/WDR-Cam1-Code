#include "SD.hpp"
#include <WiFi.h>

static SemaphoreHandle_t SDMutex = NULL;
static bool SD_send = false;
static File root;
static File currentFile;

void SD_Init() {
    if (!SD.begin()) { // if the SD card fails to start skip
        Serial.println("SD init failed");
        return;
    }

    SDMutex = xSemaphoreCreateMutex(); // create a mutx for the sd 

    xTaskCreatePinnedToCore(SD_Task,"SD Task",8192,NULL,1,NULL,1); // create a multithreading task pinned to core 1
}
void SD_StartSending() {
    if (xSemaphoreTake(SDMutex, portMAX_DELAY)) { // check the mutex is free
        // open the file and set the send variable to true
        root = SD.open("/");
        currentFile = File();
        SD_send = true;

        xSemaphoreGive(SDMutex); // give back the mutex
    }
}

void SD_StopSending() {
    SD_send = false; // stop the code from sending data
}
bool Send_File(File file) {
    if (WiFi.status() != WL_CONNECTED) { // if wifi connection failes return false
        return false;
    }

    HTTPClient http;

    String serverURL = "";// fill with relevent IP

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
            if (!currentFile) { //
                currentFile = root.openNextFile();

                if (!currentFile) {
                    root.close();
                    SD_send = false;
                    xSemaphoreGive(SDMutex);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    continue;
                }
            }

            if (currentFile) {

                String filename = String(currentFile.name());

                bool Check = Send_File(currentFile.name());

                currentFile.close();   // MUST close before delete

                if (Check) {
                    if (!SD.remove(filename)) {
                        Serial.println("Failed to delete: " + filename);
                    } 
                else {
                    Serial.println("Upload failed, keeping: " + filename);
                }
                currentFile = File(); // reset
            }

            xSemaphoreGive(SDMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

esp_err_t SD_List_Handler(httpd_req_t *req) {

    if (!xSemaphoreTake(sdMutex, pdMS_TO_TICKS(100))) {
        return httpd_resp_send_500(req);
    }

    File root = SD.open("/");
    File file = root.openNextFile();

    String json = "[";

    while (file) {
        if (json.length() > 1) json += ",";
        json += "\"" + String(file.name()) + "\"";
        file = root.openNextFile();
    }

    json += "]";

    xSemaphoreGive(sdMutex);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json.c_str(), json.length());
}


esp_err_t SD_File_Handler(httpd_req_t *req) {

    char filepath[64];
    if (httpd_req_get_url_query_str(req, filepath, sizeof(filepath)) != ESP_OK) {
        return httpd_resp_send_400(req);
    }

    if (!xSemaphoreTake(sdMutex, pdMS_TO_TICKS(100))) {
        return httpd_resp_send_500(req);
    }

    File file = SD.open(filepath);

    if (!file) {
        xSemaphoreGive(sdMutex);
        return httpd_resp_send_404(req);
    }

    httpd_resp_set_type(req, "application/octet-stream");

    char buffer[1024];
    size_t readBytes;

    while ((readBytes = file.readBytes(buffer, sizeof(buffer))) > 0) {
        httpd_resp_send_chunk(req, buffer, readBytes);
    }

    file.close();
    xSemaphoreGive(sdMutex);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


esp_err_t SD_Delete_Handler(httpd_req_t *req) {

    char filepath[64];
    if (httpd_req_get_url_query_str(req, filepath, sizeof(filepath)) != ESP_OK) {
        return httpd_resp_send_400(req);
    }

    if (!xSemaphoreTake(sdMutex, pdMS_TO_TICKS(100))) {
        return httpd_resp_send_500(req);
    }

    bool success = SD.remove(filepath);

    xSemaphoreGive(sdMutex);

    if (success) {
        return httpd_resp_sendstr(req, "Deleted");
    } else {
        return httpd_resp_send_500(req);
    }
}

