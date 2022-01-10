#include <Arduino.h>
#include <ESPTelnet.h>
#include <analogWrite.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP32Servo.h>
#include <WebServer.h>
#include <ESPmDNS.h>

//Functions declarations
void handle_OnConnect();
void hanldeWebClient();
void handle_feederOn();
void handle_feederOff();
void handle_feederAutomatic();
void handle_NotFound();
String SendHTML(uint8_t catFeederStatus);

//#include <WiFi.h> //For the server function
// Set web server port number to 80
WebServer  server(80);

// Variable to store the HTTP request
String header;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

Servo myservo;  // create servo object to control a servo
// 16 servo objects can be created on the ESP32
 
int pos = 0;    // variable to store the servo position
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33 
int servoPin = 18;

bool catFeederStatus = 0; //Variable to indicate if cat feeder is on or off

const char* ssid = "*****";
const char* password = "******";

//***************************************************************************************************
void setup() {
  Serial.begin(115200); 
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  if(!MDNS.begin("catFeeder")) {
     Serial.println("Error starting mDNS");
     return;
  }

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // My Code ************************
  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(servoPin); // attaches the servo on pin 18 to the servo object
  myservo.write(10);
  //WebServer
  server.on("/", handle_OnConnect);
  server.on("/feederOn", handle_feederOn);
  server.on("/feederOff", handle_feederOff);
  server.on("/feederAutomatic", handle_feederAutomatic);

  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
}
//***************************************************************************************************
void loop() {
  ArduinoOTA.handle();
  server.handleClient();
 
}
//***************************************************************************************************
void handle_OnConnect() {


  Serial.println("On_Connect Request Received");
  server.send(200, "text/html", SendHTML(catFeederStatus)); 
}
//***************************************************************************************************
void handle_feederOn() {

  Serial.println("FeederOn Request Received");
  catFeederStatus = 1;
  myservo.write(170);//170
  server.send(200, "text/html", SendHTML(catFeederStatus)); 
  
}
//***************************************************************************************************
void handle_feederOff() {

  Serial.println("FeederOff Request Received");
  catFeederStatus = 0;
  myservo.write(10);//10
  server.send(200, "text/html", SendHTML(catFeederStatus)); 
  
}
//***************************************************************************************************
void handle_feederAutomatic() {

  Serial.println("FeederOff Request Received");
  catFeederStatus = 1;
  server.send(200, "text/html", SendHTML(catFeederStatus)); 
  myservo.write(170);
  delay(3000);
  myservo.write(10);
  catFeederStatus = 0;
  server.send(200, "text/html", SendHTML(catFeederStatus)); 
  
}
//***************************************************************************************************

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}
//***************************************************************************************************
String SendHTML(uint8_t catFeederStatus8){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>CAT FEEDER</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #3498db;}\n";
  ptr +=".button-on:active {background-color: #2980b9;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Cat Feeder Control</h1>\n";
  ptr +="<h3>To Make Whiskers loose weight!! (Chunky Cat)</h3>\n";
  
   if(catFeederStatus8)
  {ptr +="<p>Cat Feeder Status: ON</p><a class=\"button button-off\" href=\"/feederOff\">OFF</a>\n";}
  else
  {ptr +="<p>Cat Feeder Status: OFF</p><a class=\"button button-on\" href=\"/feederOn\">ON</a>\n";}


  {ptr +="<p></p><a class=\"button button-on\" href=\"/feederAutomatic\">Automatic</a>\n";}

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
//***************************************************************************************************

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(3)){
      catFeederStatus = 1;
    }
    else{
      catFeederStatus = 2;
    }
    Serial.print(catFeederStatus);
    return "OFF";
  }
  return String();
}
//***************************************************************************************************