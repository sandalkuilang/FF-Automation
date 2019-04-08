//    name    : Fishery Assistant
//    version : 0.2
//    author  : sandalkuilang

#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "ArduinoJson.h"

#define DEBUG true

// Set web server port number to 80
AsyncWebServer server(80);
AsyncWebSocket ws("/fa");

// Configuration
StaticJsonDocument<500> global_fish_config;
StaticJsonDocument<300> global_wifi_config;
StaticJsonDocument<500> jsonDoc;

// flag of reboot
bool flag_reboot = false;

// TYPE OF DATA REQUEST
char *GET = "GET";
char *SET = "SET";

// TYPE OF DATA CONFIGURATION
const byte FISH_CONFIG = 1;
const byte WIFI_CONFIG = 2;

// TYPE OF INSTRUCTION
String FISH = "fish";
String RESET = "reset";
String WIZARD = "wizard";
String INTERVAL = "interval";
String SPREAD_POWER = "spreadPower";
String DURATION = "duration";
String REBOOT = "reboot";
String UPDATE = "update";

void onRegisterFiles()
{
  ////-- web site
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  ///// CSS and JavaScript
  server.on("/src/fontawesome-webfont.woff", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/fontawesome-webfont.woff", "application/font-woff");
  });

  server.on("/src/fontawesome-webfont.woff2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/fontawesome-webfont.woff2", "application/font-woff2");
  });

  server.on("/src/fontawesome-webfont.ttf", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/fontawesome-webfont.ttf", "application/x-font-ttf");
  });

  server.on("/src/font-awesome.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/font-awesome.min.css", "text/css");
  });

  server.on("/src/onsenui.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/onsenui.css", "text/css");
  });

  server.on("/src/onsen-comp.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/onsen-comp.min.css", "text/css");
  });

  server.on("/src/onsenui.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/onsenui.min.js", "text/javascript");
  });

  server.on("/src/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/jquery.min.js", "text/javascript");
  });

  ////-- Images
  server.on("/images/logo.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/logo.png", "images/png");
  });
  server.on("/images/fish-type.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/fish-type.png", "images/png");
  });
  server.on("/images/interval.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/interval.png", "images/png");
  });
  server.on("/images/duration.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/duration.png", "images/png");
  });
  server.on("/images/spread-power.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/spread-power.png", "images/png");
  });
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    if (DEBUG)
    {
      Serial.print("Connected: ");
      Serial.println(client->id());
    }
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    if (DEBUG)
      Serial.printf("Disconnected!\n");
  }
  else if (type == WS_EVT_ERROR)
  {
    if (DEBUG)
      Serial.printf("WebSocket Error!\n");
  }
  else if (type == WS_EVT_PONG)
  {
    if (DEBUG)
      Serial.printf("Received PONG!\n");
  }
  else if (type == WS_EVT_DATA)
  {
    ///////------ parse message
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
      }

      auto error = deserializeJson(jsonDoc, msg);

      if (error)
      {
        client->text("Error: true");
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return;
      }

      /////--- validate message
      String key = jsonDoc["key"];
      String type = jsonDoc["type"];
      String value = jsonDoc["value"];

      if (key == (WIZARD))
      {
        if (type == SET)
        {
          String _fish = jsonDoc["value"]["fish"];
          String _interval = jsonDoc["value"]["interval"];
          String _duration = jsonDoc["value"]["duration"];
          String _spreadPower = jsonDoc["value"]["spreadPower"];

          setProperty(FISH_CONFIG, FISH, _fish);
          setProperty(FISH_CONFIG, INTERVAL, _interval);
          setProperty(FISH_CONFIG, DURATION, _duration);
          setProperty(FISH_CONFIG, SPREAD_POWER, _spreadPower);
          setProperty(FISH_CONFIG, WIZARD, "1");

          saveSetting(FISH_CONFIG);

          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(FISH_CONFIG, FISH);
          client->text(result);
          Serial.println("result :");
          Serial.println(result);
        }
      }
      else if (key == (FISH))
      {
        if (type == SET)
        {
          setProperty(FISH_CONFIG, FISH, value);
          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(FISH_CONFIG, FISH);
          client->text(result);
          Serial.println("result :");
          Serial.println(result);
        }
      }
      else if (key == (INTERVAL))
      {
        if (type == SET)
        {
          setProperty(FISH_CONFIG, INTERVAL, value);
          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(FISH_CONFIG, INTERVAL);
          client->text(result);
          Serial.println("result :");
          Serial.println(result);
        }
      }
      else if (key == (SPREAD_POWER))
      {
        if (type == SET)
        {
          setProperty(FISH_CONFIG, SPREAD_POWER, value);
          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(FISH_CONFIG, SPREAD_POWER);
          client->text(result);
          Serial.println("result :");
          Serial.println(result);
        }
      }
      else if (key == (DURATION))
      {
        if (type == SET)
        {
          setProperty(FISH_CONFIG, DURATION, value);
          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(FISH_CONFIG, DURATION);
          client->text(result);
          Serial.println(result);
        }
      }
      else if (key == (UPDATE))
      {
        if (type == SET)
        {
          if (value == "fish")
          {
            saveSetting(FISH_CONFIG);
            client->text("Error: false");
          }
          else if (value == "wifi")
          {
            saveSetting(WIFI_CONFIG);
            client->text("Error: false");
          }
        }
      }
      else if (key == (RESET))
      {
        if (type == SET)
        {
          loadSetting(WIFI_CONFIG, "/master/wifi.json");
          loadSetting(FISH_CONFIG, "/master/app.json");
          saveSetting(WIFI_CONFIG);
          saveSetting(FISH_CONFIG);
          client->text("Error: false");
          Serial.println("Restore configuration...");
          reboot();
        }
      }
      else if (key == (REBOOT))
      {
        if (type == SET)
        {
          reboot();
          client->text("Error: false");
        }
      }
    }
  }
}

void loadSetting(byte typeConfiguration)
{
  if (typeConfiguration == FISH_CONFIG)
    loadSetting(FISH_CONFIG, "/app.json");
  else
    loadSetting(WIFI_CONFIG, "/wifi.json");
}

void loadSetting(byte typeConfiguration, String path)
{
  String content;
  String _path;
  if (typeConfiguration == FISH_CONFIG)
  {
    if (path != "")
      _path = path;
    else
      _path = "/app.json";

    content = readFile(_path);
    auto error = deserializeJson(global_fish_config, content);

    if (error)
    {
      Serial.print(F("deserializeJson() failed "));
      Serial.println(error.c_str());
      return;
    }
  }
  else
  {
    if (path != "")
      _path = path;
    else
      _path = "/app.json";
    content = readFile(_path);
    auto error = deserializeJson(global_wifi_config, content);

    if (error)
    {
      Serial.print(F("deserializeJson() failed : "));
      Serial.println(error.c_str());
      return;
    }
  }
}

void saveSetting(byte typeConfiguration)
{
  String jsonContent;
  if (typeConfiguration == FISH_CONFIG)
  {
    deleteFile("/app.json");
    serializeJson(global_fish_config, jsonContent);
    writeFile("/app.json", jsonContent);
    Serial.println("saving configuration, ID 1");
  }
  else
  {
    deleteFile("/wifi.json");
    serializeJson(global_wifi_config, jsonContent);
    writeFile("/wifi.json", jsonContent);
    Serial.println("saving configuration, ID 2");
  }
}

void reboot()
{
  flag_reboot = true;
}

void deleteFile(const char *path)
{
  if (!SPIFFS.exists(path))
  {
    Serial.printf("File: %s does not exist, not deleting\n", path);
    return;
  }

  Serial.printf("Deleting file: %s\n", path);

  if (SPIFFS.remove(path))
  {
    Serial.println("File deleted");
  }
  else
  {
    Serial.println("Delete failed");
  }
}

void writeFile(const char *path, String data)
{
  Serial.printf("Writing file: %s\n", path);

  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  if (file.print(data))
    Serial.println("File written");
  else
    Serial.println("Write failed");

  file.close();
}

String readFile(String path)
{
  String result = "";
  File f = SPIFFS.open(path, FILE_READ);
  if (!f)
  {
    Serial.println("Can't open SPIFFS file !\r\n");
  }
  else
  {
    while (f.available())
    {
      result += (char)f.read();
    }
    return result;
  }
  return result;
}

const char *getProperty(byte typeConfiguration, String key)
{
  if (typeConfiguration == FISH_CONFIG)
  {
    return global_fish_config[key];
  }
  else
  {
    return global_wifi_config[key];
  }
}

void setProperty(byte typeConfiguration, String key, String value)
{
  if (typeConfiguration == FISH_CONFIG)
  {
    global_fish_config[key] = value;
  }
  else
  {
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

void setup()
{
  Serial.begin(115200);

  //// Intialize web server
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Serial.println("setup web server...");
  onRegisterFiles();

  Serial.println("load configuration...");
  //// Intitialize configuration
  loadSetting(FISH_CONFIG);
  loadSetting(WIFI_CONFIG);

  if (!SPIFFS.exists("/master/app.json") && !SPIFFS.exists("/master/wifi.json"))
  {
    Serial.print("create backup configuration...");
    ///// Create backup configuration
    String appconfig;
    String wificonfig;
    serializeJson(global_fish_config, appconfig);
    serializeJson(global_wifi_config, wificonfig);
    writeFile("/master/app.json", appconfig);
    writeFile("/master/wifi.json", wificonfig);
    Serial.println("OK");
  }

  //// Setup Access Point from configuration
  const char *_ssid = getProperty(WIFI_CONFIG, "ssid");
  const char *_password = getProperty(WIFI_CONFIG, "password");
  WiFi.softAP(_ssid, _password);
  IPAddress IP = WiFi.softAPIP();

  Serial.println("setting AP (Access Point)...");
  Serial.print("SSID : ");
  Serial.println(_ssid);
  Serial.print("PASS : ");
  Serial.println(_password);
  Serial.print("AP IP address: ");
  Serial.println(IP);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();

  String author = readFile("/author");
  Serial.println("\n" + author + "\n");
}
