//Jacob Holwill 10859926
// a file to set up the streaming of the camera and gnss data and to connect to the wifi

#include "Server.hpp"

// set the ssid and password 
const char* ssid = "iPhone"; 
const char* password = "12345678";

httpd_handle_t Server = NULL;

void Cam1_Server_Init() {

  httpd_config_t Cam1_Server_Config = HTTPD_DEFAULT_CONFIG(); // create a variable to save all server info/ settings
  Cam1_Server_Config.server_port = 80; // set the webservers pot to number 80 which is standard
  Cam1_Server_Config.ctrl_port = 30000; // set the controll port to avoid conflict with any other ports

  static httpd_uri_t Base_URL = (.uri = "/", .method = HTTP_GET, .handler = Cam_Stream_Handler, .user_ctx = NULL);

  // create a variable and store the cameras URL in it for streaming and to asses the cams veiw
  static httpd_uri_t Cam1_Stream_URL = {.uri = "/stream", .method = HTTP_GET, .handler = Cam1_Stream_Update, .user_ctx = NULL}; 

  // create a variable and store the debugging/status URL in it to be able to acess relevent info
  static httpd_uri_t GPS_Status_URL = {.uri = "/status", .method = HTTP_GET, .handler = GPS_Status_Update, .user_ctx = NULL};
    
  // Start the server
  if (httpd_start(&Server, &Cam1_Server_Config) == ESP_OK) { //check everything is set up correctly start the server
    httpd_register_uri_handler(Server, &Base_URL);
    httpd_register_uri_handler(Server, &Cam1_Stream_URL); // create the cameras page on the server 
    httpd_register_uri_handler(Server, &GPS_Status_URL); // create the status page on the server
    Serial.println("Camera 1 server began"); // print to the terminal so you now the server is ready
  }
}
esp_err_t Cam_Stream_Handler(httpd_req_t *req) {
    const char* html =
        "<!DOCTYPE html>"
        "<html><body>"
        "<h1>ESP32 Camera</h1>"
        "<img src=\"/stream\">"
        "</body></html>";

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}
void WIFI_Connect(){
  
  Serial.println("");
  Serial.print("Connecting to WIFI");

  WiFi.begin(ssid, password); // connect the wifi to the set ssid and password
  WiFi.setAutoReconnect(true); // set it to auto reconnect to the wifi if it becomes disconected

  while (WiFi.status() != WL_CONNECTED) { // while wifi isnt connected
    delay(500); // wait 500ms
    Serial.print("."); // print a dot to the terminal to show its still attempting to connect
  }

  Serial.println("");
  Serial.println("WiFi connected"); // print once wifi is connected

}