//Jacob Holwill 10859926
//

#include "Cam.hpp"
#include "Arduino.h"
#include "Gps.hpp"

//Pin Definitions were taken from the XIAO ESP32-S3 Sense section for the camera webserver example code
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39
#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13
httpd_handle_t Server = NULL;

// create a funtion to handle the stream and return any esp errors and points to the http request object
static esp_err_t Cam1_Stream_Update(httpd_req_t *Server_Request) { 
    camera_fb_t * Cam1_Frame_Buffer = NULL; // creates a pointer to the frame buffer for the camera 
    esp_err_t Error_Check = ESP_OK; // used to shows the result of future operations
    size_t JPG_Buffer_Size = 0; // creates a buffer to store a jpeg image in
    uint8_t * JPG_Buffer = NULL; // creates a pointer to the JPEG image info
    char Parts_Buffer[64]; // temporary buffer to store the headers for the server

    // tells the browser its a jpeg stream with boundrys (vid) between each image
    Error_Check = httpd_resp_set_type(Server_Request, "multipart/x-mixed-replace;boundary=vid"); 

    if(Error_Check != ESP_OK) return Error_Check; // check the error check fails then exit but if not then continue

    while(true) {
        Cam1_Frame_Buffer = esp_camera_fb_get(); // captures an image containing the JPG_Buffer and JPG_Buffer_Size
        if (!Cam1_Frame_Buffer) { // if it fails to capture
            Serial.println("Camera capture failed"); // print debug line to the terminal
            Error_Check = ESP_FAIL; // set the error check to a fail
        } else { // if the camera successfuly captures
            JPG_Buffer_Size = Cam1_Frame_Buffer->len; // store the Jpeg size  
            JPG_Buffer = Cam1_Frame_Buffer->buf;// store the Jpeg image info
        }

        if(Error_Check == ESP_OK){ // check the error checker to see if the image had captured or not

            // create a header for the JPEG info
            size_t JPG_Info = snprintf((char *)Parts_Buffer, 64, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", JPG_Buffer_Size); 
            Error_Check = httpd_resp_send_chunk(Server_Request, (const char *)Parts_Buffer, JPG_Info); // sends the header chunk to the browser
        }
        
        // 
        if(Error_Check == ESP_OK){ // chek if the error check is still ok
            Error_Check = httpd_resp_send_chunk(Server_Request, (const char *)JPG_Buffer, JPG_Buffer_Size); // send the JPEG to the browser
        } 
        
        if(Error_Check == ESP_OK) { // chek if the error check is still ok
            Error_Check = httpd_resp_send_chunk(Server_Request, "\r\n--vid\r\n", 15); // send a image seperator to the browser
        }

        if(Cam1_Frame_Buffer) {
            esp_camera_fb_return(Cam1_Frame_Buffer); // returns the frame to the camera 
            Cam1_Frame_Buffer = NULL; // resets the buffer
        }
        
        if(Error_Check != ESP_OK){ // if the error check retreives an error
            break; // break from the loop
        } 
        vTaskDelay(pdMS_TO_TICKS(50)); // delay to prevent power spikes and instability and also create roughly 20fps on the camera
    }
    return Error_Check; // returns the status of the error check
}

// handler for the gnss
static esp_err_t Cam1_Status_Update(httpd_req_t *Status_Request) { // a static function that returns error for the status request
    char GNSS_Write[256]; // create a buffer for the GNSS to be able to write in
    if (xSemaphoreTake(GPS.mutex, 10) == pdTRUE) { // attempt to lock the mutex to be able to acess it and protect the GNSS data 

        // get the values for the gps
        float lat = GPS.lat ;
        float lon = GPS.lon ;
        float alt = GPS.alt ;
        int satellites = GPS.satellites ;
        int hour = GPS.hour ;
        int min = GPS.min ;
        int sec = GPS.sec ;
        const char* GNSS_Check; // ceate a check to see if the gnss is receiving anything

        if (GPS.val){ // if receiving data
            GNSS_Check = "true"; // check is true
        }
        else{ // if not receiving data
            GNSS_Check = "false"; // check is false
        }

        snprintf(GNSS_Write, sizeof(GNSS_Write),  // print the GNSS_Write buffer witht the following text while keeping the buffer size
        "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f,\"sats\":%d,\"h\":%d,\"m\":%d,\"s\":%d,\"valid\":%s}", // write this to the buffer
        lat, lon, alt, satellites, hour, min, sec, GNSS_Check); // write the data to its corrosponding % in the previous text

        xSemaphoreGive(GPS.mutex); // unlock the mutex
    } 
    else { // if mutex is unable to be locked/read
        strcpy(GNSS_Write, "{\"error\":\"GPS mutex is busy\"}"); // if the mutex is being used create this error message in the buffer
    }
    httpd_resp_set_type(Status_Request, "application/json"); // tells the browser the respons of the json
    return httpd_resp_send(Status_Request, GNSS_Write, strlen(GNSS_Write)); // sends the json back to the client
}

void Cam1_init() {
    //Pin configuration were taken from the camera webserver example code and adjusted where nessesary
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG;  // for streaming
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    esp_camera_init(&config);

    // the following is manual sensor adjustments for better stability (taken from the example code)
    sensor_t * sens = esp_camera_sensor_get(); // gets the camera type and sets it to pointer sens

    if (sens->id.PID == OV3660_PID) { // if it picks up the correct camera type do the following
        sens->set_vflip(sens, 1); // flip the camera vertically to flip it back to normal 
        sens->set_brightness(sens, -3);// lower the brightness to improve video quality
        sens->set_saturation(sens, -1);// lower the saturation to improve video quality
    }
    if (config.pixel_format == PIXFORMAT_JPEG) { // if the picture is in jpeg format 
        sens->set_framesize(sens, FRAMESIZE_QVGA); // change the camera quality to QVGA
    }
}

void Cam1_Server_Init() {
    httpd_config_t Cam1_Server_Config = HTTPD_DEFAULT_CONFIG(); // create a variable to save all server info/ settings
    Cam1_Server_Config.server_port = 80; // set the webservers pot to number 80 which is standard
    Cam1_Server_Config.ctrl_port = 30000; // set the controll port to avoid conflict with any other ports

    // create a variable and store the cameras URL in it for streaming and to asses the cams veiw
    httpd_uri_t Cam1_Stream_URL = {.uri = "/stream", .method = HTTP_GET, .handler = Cam1_Stream_Update, .user_ctx = NULL}; 

    // create a variable and store the debugging/status URL in it to be able to acess relevent info
    httpd_uri_t Cam1_Status_URL = {.uri = "/status", .method = HTTP_GET, .handler = Cam1_Status_Update, .user_ctx = NULL};
    
    // Start the server
    if (httpd_start(&Server, &Cam1_Server_Config) == ESP_OK) { //check everything is set up correctly start the server
        httpd_register_uri_handler(Server, &Cam1_Stream_URL); // create the cameras page on the server 
        httpd_register_uri_handler(Server, &Cam1_Status_URL); // create the status page on the server
        Serial.println("Camera 1 server began"); // print to the terminal so you now the server is ready
    }
}
