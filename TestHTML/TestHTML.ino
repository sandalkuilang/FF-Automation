//    name    : Smart Fish Feeder
//    version : 0.1
//    author  : sandalkuilang
 
#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h" 
#include "AsyncTCP.h"
#include "ArduinoJson.h"

#define DEBUG true

// Set web server port number to 80 
AsyncWebServer server(80); 
AsyncWebSocket ws("/sff");

// Configuration
StaticJsonDocument<500> global_fish_config;
StaticJsonDocument<300> global_wifi_config;
StaticJsonDocument<500> jsonDoc;

// flag of reboot
bool flag_reboot = false;

// TYPE OF DATA REQUEST
char* GET = "GET";
char* SET = "SET";

// TYPE OF DATA CONFIGURATION
const byte FISH_CONFIG = 1;
const byte WIFI_CONFIG = 2;

// TYPE OF INSTRUCTION
String FISH = "fish";
String INTERVAL = "interval";
String SPREAD_POWER = "spreadPower";
String DURATION = "duration";
String REBOOT = "reboot";
String UPDATE = "update";

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    if (DEBUG) {
      Serial.print("Connected: ");
      Serial.println(client->id());
    } 
  } else if(type == WS_EVT_DISCONNECT){
    if (DEBUG) Serial.printf("Disconnected!\n");  
  } else if(type == WS_EVT_ERROR){
    if (DEBUG) Serial.printf("WebSocket Error!\n"); 
  } else if(type == WS_EVT_PONG){
    if (DEBUG) Serial.printf("Received PONG!\n"); 
  } else if(type == WS_EVT_DATA){
    ///////------ parse message
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      }  
       
      
      auto error = deserializeJson(jsonDoc, msg.c_str());
        
      if (error) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return;
      }
 
      /////--- validate message
      String key = jsonDoc["key"]; 
      String type = jsonDoc["type"];
      String value = jsonDoc["value"];

      if (key == (FISH)){
        if (type == SET) {
          setProperty(FISH_CONFIG, FISH, value);  
        } else if (type == GET){
          const char* result = getProperty(FISH_CONFIG, FISH);
          client->text(result);
          Serial.println("result :");
          Serial.println(result);
        }  
      }
      else if (key == (INTERVAL)){
        if (type == SET) {
          setProperty(FISH_CONFIG, INTERVAL, value);  
        } else if (type == GET){
          const char* result = getProperty(FISH_CONFIG, INTERVAL);
          client->text(result);
          Serial.println("result :");
          Serial.println(result);
        } 
      }
      else if (key == (SPREAD_POWER)){
        if (type == SET) {
           setProperty(FISH_CONFIG, SPREAD_POWER, value);           
        } else if (type == GET){
          const char* result = getProperty(FISH_CONFIG, SPREAD_POWER);
          client->text(result);
          Serial.println("result :" );
          Serial.println(result);
        } 
      }
       else if (key == (DURATION)){
        if (type == SET) {
           setProperty(FISH_CONFIG, DURATION, value);
        } else if (type == GET){
          const char* result = getProperty(FISH_CONFIG, DURATION);
          client->text(result);
          Serial.println(result);
        } 
      }
      else if (key == (UPDATE)){
        if (type == SET) {
          if (value == "fish")
            saveConfiguration(FISH_CONFIG);
          else if (value == "wifi")
            saveConfiguration(WIFI_CONFIG);

          client->text("please reboot the device");
        }
      }
      else if (key == (REBOOT)){
        if (type == SET) {
          reboot(); 
          client->text("set flag reboot true");
        }
      } 
    }  
  }
}

void loadConfiguration(byte typeConfiguration) {
  String content;
  if (typeConfiguration == FISH_CONFIG) { 
    content = readFile("/fish.json"); 
    auto error = deserializeJson(global_fish_config, content);
      
    if (error) {
      Serial.print(F("deserializeJson() failed "));
      Serial.println(error.c_str());
      return;
    }
  }
  else { 
    content = readFile("/wifi.json");  
    auto error = deserializeJson(global_wifi_config, content);
      
    if (error) {
      Serial.print(F("deserializeJson() failed : "));
      Serial.println(error.c_str());
      return;
    }
  }
}
 
void saveConfiguration(byte typeConfiguration) {
  char* jsonContent;
  if (typeConfiguration == FISH_CONFIG)
  {
    deleteFile("/fish.json");
    serializeJson(global_fish_config, jsonContent, sizeof(jsonContent));
    writeFile("/fish.json", jsonContent);
  } else {
    deleteFile("/wifi.json");
    serializeJson(global_wifi_config, jsonContent, sizeof(jsonContent));
    writeFile("/wifi.json", jsonContent);
  }
}

void reboot() { 
  flag_reboot = true;
}

void deleteFile(const char *path) {
  if (DEBUG) Serial.print(SPIFFS.exists(path));
  if (!SPIFFS.exists(path)) {
    Serial.printf("File: %s does not exist, not deleting\n", path);
    return;
  }

  Serial.printf("Deleting file: %s\n", path);

  if (SPIFFS.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void writeFile(const char *path, const char *data) {
  Serial.printf("Writing file: %s\n", path);

  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
 
  if (file.print(data)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
 
  file.close();
}
 
String readFile(const char *path) { 
  String result = "";
  File f = SPIFFS.open(path, FILE_READ);
  if (!f) {
    Serial.println("Can't open SPIFFS file !\r\n");          
  }
  else {
    while(f.available()){
      result += (char) f.read();
    } 
    return result; 
  } 
  return result; 
}

const char* getProperty(byte typeConfiguration, String key){
  if (typeConfiguration == FISH_CONFIG){
    return global_fish_config[key];
  } 
  else {
    return global_wifi_config[key];
  } 
}

void setProperty(byte typeConfiguration, String key, String value){
   if (typeConfiguration == FISH_CONFIG){
    global_fish_config[key] = value;
  } 
  else {
    global_wifi_config[key] = value;
  }
}

void loop() 
{
  if (flag_reboot)
  { 
    ESP.restart();
  }
  delay(1000);
}
 
void setup() {
  Serial.begin(115200);
 
  //// Intialize web server
  if(!SPIFFS.begin()){
     Serial.println("An Error has occurred while mounting SPIFFS");
     return;
  }

  Serial.println("setup web server..."); 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
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
 
  server.on("/src/zepto.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/src/zepto.min.js", "text/javascript");
  });
     
  Serial.println("load configuration...");  
  //// Intitialize configuration
  loadConfiguration(FISH_CONFIG);
  loadConfiguration(WIFI_CONFIG);
 
  //// Setup Access Point from configuration
  const char* _ssid = getProperty(WIFI_CONFIG, "ssid");
  const char* _password = getProperty(WIFI_CONFIG, "password");
  WiFi.softAP(_ssid, _password);
  IPAddress IP = WiFi.softAPIP();

  Serial.println("setting AP (Access Point)..."); 
  Serial.print("SSID : "); 
  Serial.println(_ssid);
  Serial.print("PASS : "); 
  Serial.println(_password);
  Serial.print("AP IP address: ");
  Serial.println(IP);
  

  // Intitialize Server
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin(); 


  String author = readFile("/author");
  Serial.println();
  Serial.println(author);
}