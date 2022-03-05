#include <Arduino.h>
#include <analogWrite.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP32Servo.h>
#include <MDNS_Generic.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include "time.h"
#include <ESPDateTime.h>
#include "ESP32Time.h"
#include <TimeLib.h>
#include "time.h"
#include "ESPTelnet.h"

WiFiUDP udp;
MDNS mdns(udp);

// Functions declarations
void notifyClients();
tm GetLastFeedingTime();
void sendDebugMessage(String message);
// Set web server port number to 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
// Set Telnet server for remote debugging
ESPTelnet telnet;
IPAddress ip;

// Current millis count
unsigned long currentTime = millis();
// Previous millis count
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
// Variable for real time (time.h)
const char *ntpServer = "time-a-b.nist.gov";
const long gmtOffset_sec = -6 * 3600;
const int daylightOffset_sec = 3600;
struct tm timeinfo;
ESP32Time rtc;

Servo myservo; // create servo object to control a servo

int pos = 0; // variable to store the servo position

int servoPin = 18;

bool catFeederStatus = 0; // Variable to indicate if cat feeder is on or off
bool autoON = false;

String ssid = "*****";
String password = "******";
//***************************************************************************************************
void ToggleAutomaticMode()
{
  if (autoON == false)
  {
    autoON = true;
    ws.textAll("AutoON");
  }
  else
  {
    autoON = false;
    ws.textAll("AutoOFF");
  }
}
//***************************************************************************************************
void ToggleCatFeeder()
{
  if (catFeederStatus == 0)
  {
    myservo.write(170);
    catFeederStatus = 1;
  }
  else
  {
    myservo.write(10);
    catFeederStatus = 0;
  }
  ws.textAll(String(catFeederStatus));
  sendDebugMessage(F("Cat Feeder Toggled: "));
  sendDebugMessage(String(catFeederStatus));
}
//***************************************************************************************************
void FeedAutomatically()
{
  // print message to notify we are starting.
  sendDebugMessage(F("Starting Scheduled Feeding..."));
  if (catFeederStatus == 1)
  {
    sendDebugMessage(F("Error: Feeder is busy... I see it is open."));
    return;
  }
}
//***************************************************************************************************
void LogFeedingTime()
{
  File fileFeedingLog = SPIFFS.open("/FeedingLog.ini", "w");
  if (!fileFeedingLog)
  {
    sendDebugMessage("Failed to open feeding log for writing to it");
    return;
  }

  if (fileFeedingLog.println(String(rtc.getMonth() + 1) + char(32) + String(rtc.getDay()) + char(32) + String(rtc.getYear()) + char(32) + String(rtc.getHour()) + char(32) + String(rtc.getMinute())))
  {
    sendDebugMessage("Feeding date-time was logged");
    sendDebugMessage(rtc.getDateTime());
  }
  else
  {
    sendDebugMessage("Logging Feeding date-time FAILED");
  }
  fileFeedingLog.close();
  GetLastFeedingTime();
}
//***************************************************************************************************
tm GetLastFeedingTime()
{
  File fileFeedingLog = SPIFFS.open("/FeedingLog.ini", "r");
  tm lastFeeding;
  lastFeeding.tm_hour = 12;
  lastFeeding.tm_min = 12;
  lastFeeding.tm_sec = 12;
  if (!fileFeedingLog)
  {
    Serial.println("Failed to open feeding log to read it");
    return lastFeeding;
  }

  lastFeeding.tm_mon = fileFeedingLog.readStringUntil(char(32)).toInt();
  lastFeeding.tm_mday = fileFeedingLog.readStringUntil(char(32)).toInt();
  lastFeeding.tm_year = fileFeedingLog.readStringUntil(char(32)).toInt();
  lastFeeding.tm_hour = fileFeedingLog.readStringUntil(char(32)).toInt();
  lastFeeding.tm_min = fileFeedingLog.readStringUntil(char(32)).toInt();

  // lastFeeding = fileFeedingLog.readStringUntil(char(13));
  fileFeedingLog.close();
  sendDebugMessage(String(lastFeeding.tm_mon) + char(47) + String(lastFeeding.tm_mday) + char(47) + String(lastFeeding.tm_year) + char(32) + String(lastFeeding.tm_hour) + char(58) + String(lastFeeding.tm_min));
  // Stored like this: 2 6 2022 8 56

  return lastFeeding;
}
//***************************************************************************************************
void notifyClients()
{
  ws.textAll(String(catFeederStatus));
  ws.textAll("time: " + String(rtc.getDateTime()));
  if (autoON == false)
  {
    ws.textAll("AutoOFF");
  }
  else
  {
    ws.textAll("AutoON");
  }
}
//***************************************************************************************************
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    sendDebugMessage(F("Message Received: "));
    sendDebugMessage((char *)data);
    if (strcmp((char *)data, "manual_toggle") == 0)
    {
      // Toggle feeder position
      ToggleCatFeeder();
    }
    else if (strcmp((char *)data, "auto_toggle") == 0)
    {
      ToggleAutomaticMode();
    }
  }
}

//************************************************************************************************

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
 
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    notifyClients();
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
    if (catFeederStatus == 0)
      return "OFF";
    else
      return "ON";
  }

  else if (var == "BUTTON")
  {
    if (catFeederStatus == 0)
      return "TURN ON";
    else
      return "TURN OFF";
  }
  else if (var == "TIME")
  {
    return rtc.getDateTime();
  }
  return "";
}
//***************************************************************************************************
void printLocalTime()
{

  if (!getLocalTime(&timeinfo))
  {
    sendDebugMessage("Failed to obtain time");
    return;
  }
  rtc.setTimeStruct(timeinfo);
  sendDebugMessage(rtc.getDateTime());
}
//***************************************************************************************************
void onTelnetConnect(String ip)
{
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" connected");
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("(Use ^] + q  to disconnect.)");
}
//***************************************************************************************************
void onTelnetConnectionAttempt(String ip)
{
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" tried to connected");
  //***************************************************************************************************
}
void setupTelnet()
{
  // passing on functions for various telnet events
  telnet.onConnect(onTelnetConnect);
  telnet.onConnectionAttempt(onTelnetConnectionAttempt);

  // passing a lambda function
  telnet.onInputReceived([](String str)
                         {
    // checks for a certain command
    if (str == "ping") {
      telnet.println("> pong");
      Serial.println("- Telnet: pong");
    } });

  Serial.print("- Telnet: ");
  if (telnet.begin())
  {
    Serial.println("running");
  }
  else
  {
    Serial.println("error.");
  }
}
//***************************************************************************************************
void sendDebugMessage(String message)
{
  Serial.println(message);
  telnet.println(message);
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

  // Reads the WIFI Password from the SPISS
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

  WiFi.setHostname("Cat_Feeder_ESP32");
  Serial.println(ssid);
  WiFi.begin(ssid.c_str(), password.c_str());
  delay(3000);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  // Start Telnet
  setupTelnet();
  ip = WiFi.localIP();

  // Starts mDNS service to recognize this device as catfeeder.local
  String hostname = "catfeeder";

  Serial.print("Registering mDNS hostname: ");
  Serial.println(hostname);
  Serial.print("To access, using ");
  Serial.print(hostname);
  Serial.println(".local");
  mdns.begin(WiFi.localIP(), hostname.c_str());

  // Arduino over the are sync code
  ArduinoOTA
      .onStart([]()
               {
                 String type;
                 if (ArduinoOTA.getCommand() == U_FLASH)
                   type = "sketch";
                 else // U_SPIFFS
                   type = "filesystem";

                 // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                 Serial.println("Start updating " + type); })
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
                   Serial.println("End Failed"); });

  ArduinoOTA.begin();

  // Initialize the servo and associated timers. The servo is sent to default position.
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50); // standard 50 hz servo
  myservo.attach(servoPin);   // attaches the servo on pin 18 to the servo object
  myservo.write(10);

  // Start and configue the WebServer

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
  // Get the real time from the server
  DateTime.setServer("time-a-b.nist.gov");
  DateTime.setTimeZone("CST-6");
  DateTime.begin(/* timeout param */);
  if (!DateTime.isTimeValid())
  {
    Serial.println("Failed to get time from server.");
  }

  Serial.print(F("Time from ESP32TIMELIB: "));
  Serial.println(DateTime.toString());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Send our messages to the console stating we are ready to rock and roll!
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  printLocalTime();
  Serial.println("HTTP server started");
}

//***************************************************************************************************
void loop()
{

  ArduinoOTA.handle();
  telnet.loop();
  ws.cleanupClients();
  if (autoON == true)
  {
    delay(1000);
    FeedAutomatically();
    GetLastFeedingTime();
  }

  String serialMessage;
  while (Serial.available())
  {
    serialMessage = Serial.readString();
    sendDebugMessage("command received, logging time");
    LogFeedingTime();
    delay(2000);
  }
}

//***************************************************************************************************
