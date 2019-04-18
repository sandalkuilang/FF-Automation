//    name    : Fishery Assistant
//    version : 0.2
//    author  : sandalkuilang

#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include "FS.h"
#include "ArduinoJson.h"
#include "Wire.h"
#include "RtcDS3231.h"

RtcDS3231<TwoWire> Rtc(Wire);

void loadSetting(byte typeConfiguration);
void loadSetting(byte typeConfiguration, String path);
void deleteFile(const char *path);
const char *getProperty(byte typeConfiguration, String key);
void onRegisterFiles();
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
String readFile(String path);
void writeFile(const char *path, String data);
void reboot();
void saveSetting(byte typeConfiguration);
void setProperty(byte typeConfiguration, String key, String value);
void getDateTime(const RtcDateTime &dt);

#define DEBUG true

// Set web server port number to 80
AsyncWebServer *server;
AsyncWebSocket *ws;

// Configuration
StaticJsonDocument<300> global_app_config;
StaticJsonDocument<300> global_wifi_config;
StaticJsonDocument<300> jsonDoc;

// TYPE OF DATA REQUEST
const char *GET = "GET";
const char *SET = "SET";

// TYPE OF DATA CONFIGURATION
const byte APP_CONFIG = 1;
const byte WIFI_CONFIG = 2;

// TYPE OF INSTRUCTION
char const *LANGUAGE = "language";
char const *WIZARD = "wizard";
char const *APP = "app";
char const *INTERVAL = "interval";
char const *SPREAD_POWER = "spreadPower";
char const *DURATION = "duration";
char const *WAKEUP = "wakeup";
char const *SLEEP = "sleep";
char const *RESET = "reset";
char const *REBOOT = "reboot";
char const *UPDATE = "update";

void onRegisterFiles()
{
  ////-- web site
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server->on("/settings.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/settings.html", "text/html");
  });

  ///// CSS and JavaScript
  server->on("/src/pace.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/pace.min.js", "text/javascript");
  });

  server->on("/src/onsenui-core.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/onsenui-core.css", "text/css");
  });

  server->on("/src/onsen-comp.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/onsen-comp.min.css", "text/css");
  });

  server->on("/src/framework7.bundle.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/framework7.bundle.min.css", "text/css");
  });

  server->on("/src/framework7.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/framework7.bundle.min.js", "text/javascript");
  });

  server->on("/src/onsenui.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/onsenui.min.js", "text/javascript");
  });

  server->on("/src/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/jquery.min.js", "text/javascript");
  });

  server->on("/src/framework7-icons.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/src/framework7-icons.css", "text/css");
  });

  server->on("/fonts/Framework7Icons.ttf", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/fonts/Framework7Icons.ttf", "application/font-ttf");
  });

  ////-- Images
  server->on("/images/logo.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/logo.png", "images/png");
  });
  server->on("/images/time.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/time.png", "images/png");
  });
  server->on("/images/language.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/language.png", "images/png");
  });
  server->on("/images/interval.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/interval.png", "images/png");
  });
  server->on("/images/duration.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/duration.png", "images/png");
  });
  server->on("/images/spread-power.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/spread-power.png", "images/png");
  });
  server->on("/images/start.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/start.png", "images/png");
  });
  server->on("/images/end.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/end.png", "images/png");
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
      Serial.print("Free Heap : ");
      Serial.println(ESP.getFreeHeap());
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

      if (DEBUG)
      {
        Serial.print("key :");
        Serial.println(key);
        Serial.print("type :");
        Serial.println(type);
        Serial.print("value :");
        Serial.println(value);
      }

      if (key == (WIZARD))
      {
        Serial.println(key);
        if (type == SET)
        {
          const char *_language = jsonDoc["value"]["language"];
          unsigned long _epoch = jsonDoc["value"]["epoch"];
          const char *_interval = jsonDoc["value"]["interval"];
          const char *_spreadPower = jsonDoc["value"]["spreadPower"];
          const char *_duration = jsonDoc["value"]["duration"];
          const char *_wakeup = jsonDoc["value"]["wakeup"];
          const char *_sleep = jsonDoc["value"]["sleep"];

          setProperty(APP_CONFIG, LANGUAGE, _language);
          setProperty(APP_CONFIG, INTERVAL, _interval);
          setProperty(APP_CONFIG, DURATION, _duration);
          setProperty(APP_CONFIG, SPREAD_POWER, _spreadPower);
          setProperty(APP_CONFIG, WAKEUP, _wakeup);
          setProperty(APP_CONFIG, SLEEP, _sleep);
          setProperty(APP_CONFIG, WIZARD, "1");

          ///// set RTC
          Rtc.SetDateTime(_epoch);
          if (!Rtc.IsDateTimeValid())
            Serial.println("Real Time Clock failed to update");

          saveSetting(APP_CONFIG);

          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(APP_CONFIG, WIZARD);
          client->text(result);
          Serial.print("result : ");
          Serial.println(result);
        }
      }
      else if (key == (APP))
      {
        if (type == GET)
        {
          String appConfig;
          serializeJson(global_app_config, appConfig);
          client->text(appConfig);
          Serial.print("result : ");
          Serial.println(appConfig);
        }
      }
      else if (key == (LANGUAGE))
      {
        if (type == SET)
        {
          setProperty(APP_CONFIG, LANGUAGE, value);
          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(APP_CONFIG, LANGUAGE);
          client->text(result);
          Serial.println("result : ");
          Serial.println(result);
        }
      }
      else if (key == (INTERVAL))
      {
        if (type == SET)
        {
          setProperty(APP_CONFIG, INTERVAL, value);
          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(APP_CONFIG, INTERVAL);
          client->text(result);
          Serial.println("result : ");
          Serial.println(result);
        }
      }
      else if (key == (SPREAD_POWER))
      {
        if (type == SET)
        {
          setProperty(APP_CONFIG, SPREAD_POWER, value);
          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(APP_CONFIG, SPREAD_POWER);
          client->text(result);
          Serial.println("result : ");
          Serial.println(result);
        }
      }
      else if (key == (DURATION))
      {
        if (type == SET)
        {
          setProperty(APP_CONFIG, DURATION, value);
          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(APP_CONFIG, DURATION);
          client->text(result);
          Serial.println(result);
        }
      }
      else if (key == (WAKEUP))
      {
        if (type == SET)
        {
          setProperty(APP_CONFIG, WAKEUP, value);
          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(APP_CONFIG, WAKEUP);
          client->text(result);
          Serial.println(result);
        }
      }
      else if (key == (SLEEP))
      {
        if (type == SET)
        {
          setProperty(APP_CONFIG, SLEEP, value);
          client->text("Error: false");
        }
        else if (type == GET)
        {
          const char *result = getProperty(APP_CONFIG, SLEEP);
          client->text(result);
          Serial.println(result);
        }
      }
      else if (key == (UPDATE))
      {
        if (type == SET)
        {
          if (value == (char *)"fish")
          {
            saveSetting(APP_CONFIG);
            client->text("Error: false");
          }
          else if (value == (char *)"wifi")
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
          if (SPIFFS.exists("/master/app.json") && SPIFFS.exists("/master/wifi.json"))
          {
            Serial.print("restore backup configuration...");
            String app = readFile("/master/app.json");
            String wifi = readFile("/master/wifi.json");
            deleteFile("/app.json");
            deleteFile("/wifi.json");
            writeFile("/app.json", app);
            writeFile("/wifi.json", wifi);
            Serial.println("OK");
            reboot();
          }
          client->text("error: true");
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
  if (typeConfiguration == APP_CONFIG)
    loadSetting(APP_CONFIG, "/app.json");
  else
    loadSetting(WIFI_CONFIG, "/wifi.json");
}

void loadSetting(byte typeConfiguration, String path)
{
  String content;
  String _path;
  if (typeConfiguration == APP_CONFIG)
  {
    if (path != "")
      _path = path;
    else
      _path = "/app.json";

    content = readFile(_path);
    auto error = deserializeJson(global_app_config, content);

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
  if (typeConfiguration == APP_CONFIG)
  {
    deleteFile("/app.json");
    serializeJson(global_app_config, jsonContent);
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
  ESP.restart();
}

void deleteFile(const char *path)
{
  if (!SPIFFS.exists(path))
  {
    Serial.printf("File: %s does not exist, not deleting\n", path);
    return;
  }

  if (!SPIFFS.remove(path))
  {
    Serial.println("Delete failed");
  }
}

void writeFile(const char *path, String data)
{
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  if (!file.print(data))
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
  }
  f.close();
  return result;
}

const char *getProperty(byte typeConfiguration, String key)
{
  if (typeConfiguration == APP_CONFIG)
  {
    return global_app_config[key];
  }
  else
  {
    return global_wifi_config[key];
  }
}

void setProperty(byte typeConfiguration, String key, String value)
{
  if (typeConfiguration == APP_CONFIG)
  {
    global_app_config[key] = value;
  }
  else
  {
    global_wifi_config[key] = value;
  }
}

void loop()
{
}

void setup()
{
  Serial.begin(115200);

  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  //// Intialize web server
  server = new AsyncWebServer(80);
  ws = new AsyncWebSocket("/fa");

  Rtc.Begin();

  Serial.println("setup web server...");
  onRegisterFiles();

  Serial.println("load configuration...");
  //// Intitialize configuration
  loadSetting(APP_CONFIG);
  loadSetting(WIFI_CONFIG);

  if (DEBUG)
  {
    String appconfig;
    String wificonfig;
    serializeJson(global_app_config, appconfig);
    serializeJson(global_wifi_config, wificonfig);
    Serial.println(appconfig);
    Serial.println(wificonfig);
  }

  if (!Rtc.GetIsRunning())
  {
    Serial.println("Real Time Clock was not actively running, starting now...");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  Serial.print("Date time : ");
  getDateTime(now);

  uint64_t chipid = ESP.getEfuseMac();
  Serial.printf("Chip ID = %04X", (uint16_t)(chipid >> 32));
  Serial.printf("%08X\n", (uint32_t)chipid);

  ///// override Wi-Fi ssid name with chipid if NULL
  char ssid[15];
  snprintf(ssid, 15, "%08X", (uint32_t)chipid);
  const char *ssidProp = getProperty(WIFI_CONFIG, "ssid");
  const char *empty = "-";
  if (sizeof(ssidProp) == sizeof(empty))
  {
    String wificonfig;
    setProperty(WIFI_CONFIG, "ssid", ssid);
    deleteFile("/wifi.json");
    serializeJson(global_wifi_config, wificonfig);
    writeFile("/wifi.json", wificonfig);
    ssidProp = ssid;
    Serial.println("save wifi config...");
  }

  //// Setup Access Point from configuration
  const char *_ssid = ssidProp;
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

  if (!SPIFFS.exists("/master/app.json") && !SPIFFS.exists("/master/wifi.json"))
  {
    Serial.print("create backup configuration...");
    ///// Create backup configuration
    String appconfig;
    String wificonfig;
    serializeJson(global_app_config, appconfig);
    serializeJson(global_wifi_config, wificonfig);
    writeFile("/master/app.json", appconfig);
    writeFile("/master/wifi.json", wificonfig);
    Serial.println("OK");
  }

  ////  SPIFFS
  Serial.println("Free SPIFFS : ");
  Serial.println(SPIFFS.totalBytes() - SPIFFS.usedBytes());

  String author = readFile("/author");
  Serial.println("\n" + author + "\n");

  server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server->onNotFound(
      [](AsyncWebServerRequest *request) { request->send(404); });

  ws->onEvent(onWsEvent);
  server->addHandler(ws);
  server->begin();

  delay(500);
}

#define countof(a) (sizeof(a) / sizeof(a[0]))
void getDateTime(const RtcDateTime &dt)
{
  char datestring[20];
  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
  Serial.println(datestring);
}