/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

// Load Wi-Fi library
#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"

// Replace with your network credentials
const char* ssid     = "Fish-Food-Automation";
const char* password = "123456789";

// Set web server port number to 80
AsyncWebServer server(80);
 
void setup() {
  Serial.begin(115200);
  
  
  if(!SPIFFS.begin()){
     Serial.println("An Error has occurred while mounting SPIFFS");
     return;
  }

  server.on("/index", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
 
  server.on("/src/onsenui.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/src/onsenui.css", "text/css");
  });
    
  server.on("/src/onsen-comp.min.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/src/onsen-comp.min.css", "text/css");
  });

  server.on("/src/onsenui.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/src/onsenui.min.js", "text/javascript");
  });
 
  server.on("/src/jquery-3.3.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/src/jquery-3.3.1.min.js", "text/javascript");
  });
 
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
}

void loop(){
    
}
