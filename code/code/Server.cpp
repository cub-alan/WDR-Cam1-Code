//Jacob Holwill 10859926
// a file to set up the streaming of the camera and gnss data and to connect to the wifi

#include "Server.hpp"
#include "SD.hpp"


// set the ssid and password 
const char* ssid = "iPhone"; 
const char* password = "12345678";

httpd_handle_t Server = NULL;
esp_err_t Cam_Stream_Handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;

    res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    if(res != ESP_OK) return res;

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    while(true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        // Send boundary
        res = httpd_resp_send_chunk(req, "--frame\r\n", 9);

        // Send headers
        if(res == ESP_OK) {
            char part_buf[128];
            int len = snprintf(part_buf, sizeof(part_buf),"Content-Type: image/jpeg\r\n" "Content-Length: %u\r\n\r\n",fb->len);
            res = httpd_resp_send_chunk(req, part_buf, len);
        }

        // Send image
        if(res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        }

        // End frame
        if(res == ESP_OK) {
            res = httpd_resp_send_chunk(req, "\r\n", 2);
        }

        esp_camera_fb_return(fb);

        if(res != ESP_OK) break;

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    return res;
}

void Cam1_Server_Init() {

  httpd_config_t Cam1_Server_Config = HTTPD_DEFAULT_CONFIG(); // create a variable to save all server info/ settings
  Cam1_Server_Config.server_port = 80; // set the webservers pot to number 80 which is standard

  // create a variable and store the cameras URL in it for streaming and to asses the cams veiw
  static httpd_uri_t Stream_URI = {.uri = "/stream1", .method = HTTP_GET, .handler = Cam_Stream_Handler, .user_ctx  = NULL}; 

  // create a variable and store the debugging/status URL in it to be able to acess relevent info
  static httpd_uri_t GPS_Status_URI = {.uri = "/status", .method = HTTP_GET, .handler = GPS_Status_Update, .user_ctx = NULL};

  static httpd_uri_t SD_List_URI = {.uri = "/list",.method = HTTP_GET,.handler = SD_List_Handler,.user_ctx = NULL};
  static httpd_uri_t SD_File_URI = {.uri = "/file",.method = HTTP_GET,.handler = SD_File_Handler,.user_ctx = NULL};
  static httpd_uri_t SD_Delete_URI = {.uri = "/delete",.method = HTTP_GET,.handler = SD_Delete_Handler,.user_ctx = NULL};
    
  // Start the server
  if (httpd_start(&Server, &Cam1_Server_Config) == ESP_OK) { //check everything is set up correctly start the server
    httpd_register_uri_handler(Server, &Stream_URI); // create the cameras page on the server 
    httpd_register_uri_handler(Server, &GPS_Status_URI); // create the status page on the server
    httpd_register_uri_handler(Server, &SD_List_URI);
    httpd_register_uri_handler(Server, &SD_File_URI);
    httpd_register_uri_handler(Server, &SD_Delete_URI);
    Serial.println("Camera 1 server began"); // print to the terminal so you now the server is ready
  }
}
void WIFI_Connect(){
    Serial.print("Connecting to WiFi");

    WiFi.begin(ssid, password);
    WiFi.setSleep(false);

    unsigned long WIFI_Boot_Timer = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - WIFI_Boot_Timer <5000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n WiFi connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n WiFi failed now using SD mode");
    }
}