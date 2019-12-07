#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h> 
#include <DNSServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>

ESP8266WebServer server(80);
String temperatureGet;

#define ONEWIREBUS 0 // GPIO where the DS18B20 is connected to



// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONEWIREBUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

byte temperature = 0;

//Handles http request
void handleRoot() {
  server.send(200, "text/html", "<!DOCTYPE html><html><head><style>form input {appearance: button;padding: 10px;height: 100px;width: 100px;text-decoration: none;color: initial;}</style></head><body><h2>Esp8266 Relay Control</h2><form action='/ON'><input type='submit' value='AC' /></form><form action='/OFF'><input type='submit' value='KAPAT' /></form></body></html>");
}


void initWFM(){
  WiFiManager wifiManager;
  wifiManager.autoConnect("termostat");
  Serial.println("connected... Network");
  }
  
void handleON() {
     
     server.send(200, "text/plain", "OK");
     Serial.println("Request ON");
}

void handleOFF(){
    
     server.send(200, "text/plain", "OK");
     Serial.println("Request OFF");
}

void handleTemp(){
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  int tempCa = (int) temperatureC;
  server.send(200, "text/plain" , String(tempCa) );
  }

void handleTempSet(){

}

void setup() {
  initWFM();
  sensors.begin();
  Serial.begin(115200);
  EEPROM.begin(1);
 
  delay(10);

  Serial.println();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());   //You can get IP address assigned to ESP

  if (WiFi.status() == WL_CONNECTED) //If WiFi connected to hot spot then start mDNS
  {
    if (MDNS.begin("termostat")) {  //Start mDNS with name termostat
      Serial.println("MDNS started");
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
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  
  server.on("/", HTTP_GET, handleRoot);  //Associate handler function to path
  server.on("/ON", HTTP_GET, handleON);
  server.on("/OFF",HTTP_GET,handleOFF);
  server.on("/TEMP",HTTP_GET,handleTemp);
  server.on("/TEMPSET",handleTempSet);
  server.begin();                           //Start server
  Serial.println("HTTP server started");
  MDNS.addService("http", "tcp", 80);
}

// the loop function runs over and over again forever
void loop() {
  server.handleClient();
  MDNS.update();
  ArduinoOTA.handle();
}
