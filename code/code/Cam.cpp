//Jacob Holwill 10859926
//The point of this file is to initialise and run the code to controll and stream the camera

// include the hpp file so it has acess to all nessesary librarys
#include "Cam.hpp"

bool camera_initialized = false; // create a initialisation checker and set it to false

// create the initialiseation function for the camera 
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
    
    // Initialize as QQVGA Jpeg for efficiency and stability when initialising
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QQVGA; // start at lower fram size 

    if (psramFound()) { // if Psram is active
        // increase to QVGA with an improved quality and 
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 15;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_LATEST;
        Serial.println("PSRAM working");
    }   
     else { // if no Psram
        //initialise to QVGA at a lower qulity
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 15;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
        Serial.println("PSRAM failed");
    }

    esp_err_t err = esp_camera_init(&config); // create esp error code fetcher
    if (err != ESP_OK) { // if init failed print the error and set keep init at false
        Serial.printf("Camera init fail: 0x%x\n", err);
        camera_initialized = false;
        return;
    }
    else{ // if init succede set the variable to true
        Serial.println("Camera init succeeded");
        camera_initialized = true;
    }

    // Wait a moment for the sensor to stabilize
    delay(500);

    sensor_t * sens = esp_camera_sensor_get();  // get the senso for the camera module
    if (sens) {
        // set the values for the various camera specs
        sens->set_vflip(sens, 0);
        sens->set_hmirror(sens, 0);
        sens->set_brightness(sens, -2);
        sens->set_contrast(sens, 1); 
    }

    camera_fb_t * fb = esp_camera_fb_get(); // get the matrix for the current frame of the camera 
    if (!fb) { // if the matrix is empty or not fetched
        Serial.println("CAMERA CAPTURE FAILED");
    }   
    else { // if matrix recieved
        esp_camera_fb_return(fb); // set fb as the frame
    }
}
