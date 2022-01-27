#include <Arduino.h>
#include <ESPTelnet.h>
#include <analogWrite.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESP32Servo.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"

//Functions declarations

//#include <WiFi.h> //For the server function
// Set web server port number to 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

Servo myservo; // create servo object to control a servo

int pos = 0; // variable to store the servo position

int servoPin = 18;

bool catFeederStatus = 0; //Variable to indicate if cat feeder is on or off

String ssid = "*****";
String password = "******";

//***************************************************************************************************
void notifyClients()
{
  ws.textAll(String("Something"));
}
//***************************************************************************************************
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    if (strcmp((char *)data, "toggle") == 0)
    {
      Serial.println(F("Message Received: "));
      Serial.println((char *)data);

      notifyClients();
    }
  }
}
//***************************************************************************************************

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}
//***************************************************************************************************
void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

//***************************************************************************************************
String processor(const String &var)
{
  Serial.println(var);
  if (var == "STATUS")
  {
    Serial.println(F("STATUSITO"));
    return "STATUSITO";
  }
  
  else if (var == "BUTTON")
  {
    Serial.println(F("BUTTONCITO"));
    return "BUTTONCITO";
  }
}
//***************************************************************************************************
void setup()
{
  Serial.begin(115200);
  Serial.println(F("WELCOME   Booting..."));
  // Initialize the internal SPISS memory on the ESP32
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //Reads the WIFI Password from the SPISS
  File file = SPIFFS.open("/System.ini");
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  while (file.available())
  {
    String systemInfo = file.readStringUntil(char(34));
    ssid = file.readStringUntil(char(34));
    systemInfo = file.readStringUntil(char(34));
    password = file.readStringUntil(char(34));
  }

  file.close();
  // Establish WiFi connections.
  WiFi.mode(WIFI_STA);
  Serial.println(ssid);
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  //Arduino over the are sync code
  ArduinoOTA
      .onStart([]()
               {
                 String type;
                 if (ArduinoOTA.getCommand() == U_FLASH)
                   type = "sketch";
                 else // U_SPIFFS
                   type = "filesystem";

                 // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                 Serial.println("Start updating " + type);
               })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
                 Serial.printf("Error[%u]: ", error);
                 if (error == OTA_AUTH_ERROR)
                   Serial.println("Auth Failed");
                 else if (error == OTA_BEGIN_ERROR)
                   Serial.println("Begin Failed");
                 else if (error == OTA_CONNECT_ERROR)
                   Serial.println("Connect Failed");
                 else if (error == OTA_RECEIVE_ERROR)
                   Serial.println("Receive Failed");
                 else if (error == OTA_END_ERROR)
                   Serial.println("End Failed");
               });

  ArduinoOTA.begin();

  // Initialize the servo and associated timers. The servo is sent to default position.
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50); // standard 50 hz servo
  myservo.attach(servoPin);   // attaches the servo on pin 18 to the servo object
  myservo.write(10);

  //Start and configue the WebServer

  initWebSocket();
  // Web Server Root URL
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/Index.html", String(), false, processor); });
  server.serveStatic("/", SPIFFS, "/");
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/Style.css", "text/css"); });
  server.on("/Scripts.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/Scripts.js", "text/js"); });
  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/bootstrap.min.css", "text/css"); });

  server.begin();

  //Send our messages to the console stating we are ready to rock and roll!
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("HTTP server started");
}

//***************************************************************************************************
void loop()
{
  ArduinoOTA.handle();
  ws.cleanupClients();
}

//***************************************************************************************************
