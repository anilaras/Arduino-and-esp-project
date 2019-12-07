#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h> 
#include <DNSServer.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

#define IRLED 4  // ESP8266 GPIO pin to use. Recommended: 4 (D2).

IRsend irsend(IRLED);  // Set the GPIO to be used to sending the message.

ESP8266WebServer server(80);
String irCodeGet;
//Handles http request
void handleRoot() {
  server.send(200, "text/html", F("<!DOCTYPE html><html><head> <style> button { padding: 10px 10px 10px 10px; min-width: 90px; border-radius: 4px; transition-duration: 0.4s; } .red{ background: red; color: white; } .blue{ background: blue; color: white; } .green{ background: #4CAF50; color: white; } button:hover{ background-color: rgb(0, 0, 0); color: white; } </style></head><body> <center> <h2>Esp8266 RGB BULB Control</h2> <button type='button' onclick='window.location.href = \"/ON\";'>ON</button> <button type='button' onclick='window.location.href = \"/OFF\";'>OFF</button> <button class='red' type='button' onclick='window.location.href = \"/RED\";'>RED</button> <button class='green' type='button' onclick='window.location.href = \"/GREEN\";'>GREEN</button> <button class='blue' type='button' onclick='window.location.href = \"/BLUE\";'>BLUE</button> <button type='button' onclick='window.location.href = \"/WHITE\";'>WHITE</button> <hr> <form action='/IR'> IRCODE<br> <input type='text' name='ir'> <br><br> <input type='submit' value='Submit'> </form>  </form></center></body></html>"));
}


void initWFM(){
  WiFiManager wifiManager;
  wifiManager.autoConnect("termostat");
  }
  
void handleON() {
     irsend.sendNEC(0xFFE01F);
     server.send(200, "text/plain", "OK");
}

void handleOFF(){
     irsend.sendNEC(0xFF609F);
     server.send(200, "text/plain", "OK");
}

void handleRED(){
     irsend.sendNEC(0xFF10EF);
     server.send(200, "text/plain", "OK");
}

void handleGREEN(){
     irsend.sendNEC(0xFF906F);
     server.send(200, "text/plain", "OK");
}

void handleBLUE(){
     irsend.sendNEC(0xFF50AF);
     server.send(200, "text/plain", "OK");
}

void handleWHITE(){
     irsend.sendNEC(0xFFC03F);
     server.send(200, "text/plain", "OK");
}


void handleIR(){
 if ( ! server.hasArg("ir")
       || server.arg("ir") == NULL) { // If the POST request doesn't have temp data
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }
  else {
      irCodeGet = server.arg("ir");
      int intcast = irCodeGet.toInt();
      irsend.sendNEC(intcast);
  }
}

void setup() {
  initWFM();
  EEPROM.begin(1);
  irsend.begin();
  delay(10);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) //If WiFi connected to hot spot then start mDNS
  {
    if (MDNS.begin("irremote")) {  //Start mDNS with name termostat
      
    }
  }
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    
  });
  ArduinoOTA.onEnd([]() {
    
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    
  });
  ArduinoOTA.onError([](ota_error_t error) {
   
  });
  ArduinoOTA.begin();
  
  server.on("/", HTTP_GET, handleRoot);  //Associate handler function to path
  server.on("/ON", HTTP_GET, handleON);
  server.on("/OFF",HTTP_GET,handleOFF);
  server.on("/RED",HTTP_GET,handleRED);
  server.on("/GREEN",HTTP_GET,handleGREEN);
  server.on("/BLUE",HTTP_GET,handleBLUE);
  server.on("/WHITE",HTTP_GET,handleWHITE);
  server.on("/IR",handleIR);
  server.begin();                           //Start server

  MDNS.addService("http", "tcp", 80);
}

// the loop function runs over and over again forever
void loop() {
  server.handleClient();
  MDNS.update();
  ArduinoOTA.handle();
}
