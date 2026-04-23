//Jacob Holwill 10859926
// a file to set up the streaming of the camera and gnss data and to connect to the wifi

#include "Server.hpp"
#include "SD.hpp"

// set the ssid and password 
//const char* ssid = "Robot-Stu1"; 
//const char* password = "WANUT8PTODAY";

//testing WIFI
const char* ssid = "iPhone"; 
const char* password = "12345678";

httpd_handle_t Server = NULL; // create the server handle and set it to null

esp_err_t Cam_Stream_Handler(httpd_req_t *req) { // function that runs when cam stream  is open
    camera_fb_t * fb = NULL; // get the frame matrix from the camera

    esp_err_t Status = ESP_OK; // check if matrix is retreived

    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame"); // creates the continuous stream
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // allows different devices to access the webserver

    char json[128];

    while(true) { 
        fb = esp_camera_fb_get(); // take a capture of the image

        if (!fb) { // if the image isnt received exit loop
            Serial.println("Camera capture failed");
            Status = ESP_FAIL;
            break;
        }

        double lat=0;
        double lon=0;
        float alt=0;
        int sats=0;
        bool valid=false;

        if (xSemaphoreTake(GPS.mutex, 0)) {
            lat = GPS.lat;
            lon = GPS.lon;
            alt = GPS.alt;
            sats = GPS.satellites;
            valid = GPS.Fix_Val;
            xSemaphoreGive(GPS.mutex);
        }

        snprintf(json, sizeof(json),"{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f,\"sats\":%d,\"valid\":%d}",lat, lon, alt, sats, valid);
        httpd_resp_send_chunk(req, "--frame\r\n", 9); // create new frame marker

        char header[256];
        int len = snprintf(header, sizeof(header),"Content-Type: image/jpeg\r\n""Content-Length: %u\r\n""X-GNSS: %s\r\n\r\n",fb->len, json);

        httpd_resp_send_chunk(req, header, len);
        httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        httpd_resp_send_chunk(req, "\r\n", 2);

        esp_camera_fb_return(fb); // return the frame 

        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void Cam1_Server_Init() {

  httpd_config_t Cam1_Server_Config = HTTPD_DEFAULT_CONFIG(); // create a variable to save all server info/ settings
  Cam1_Server_Config.server_port = 80; // set the webservers pot to number 80 which is standard
  config.stack_size = 8192;
  // create a variable and store the cameras URL in it for streaming and to asses the cams veiw
  static httpd_uri_t Stream_URI = {.uri = "/stream1", .method = HTTP_GET, .handler = Cam_Stream_Handler, .user_ctx  = NULL}; 

    // create variables and store the SD URLs in it to be able to acess relevent info
  static httpd_uri_t SD_List_URI = {.uri = "/list",.method = HTTP_GET,.handler = SD_List_Handler,.user_ctx = NULL};
  static httpd_uri_t SD_File_URI = {.uri = "/file",.method = HTTP_GET,.handler = SD_File_Handler,.user_ctx = NULL};

    
  // Start the server
  if (httpd_start(&Server, &Cam1_Server_Config) == ESP_OK) { //check everything is set up correctly start the server
    httpd_register_uri_handler(Server, &Stream_URI); // create the cameras page on the server 
    // create the SD pages on the server
    httpd_register_uri_handler(Server, &SD_List_URI);
    httpd_register_uri_handler(Server, &SD_File_URI);
    Serial.println("Camera 1 server began"); // print to the terminal so you now the server is ready
  }
}
void WIFI_Connect(){
    Serial.print("Connecting to WiFi");

    WiFi.begin(ssid, password);// stat the wifi connection
    WiFi.setSleep(false); // stop sleep for impoved performance

    unsigned long WIFI_Boot_Timer = millis(); // creates a variable to store time for real time functions

    // try connecting for 5 passes
    while (WiFi.status() != WL_CONNECTED && millis() - WIFI_Boot_Timer <5000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) { // if connected
        Serial.println("\n WiFi connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP()); //prin the IP to the terminal
    } else { // if not connected swap to SD usage
        Serial.println("\n WiFi failed now using SD mode");
    }
}