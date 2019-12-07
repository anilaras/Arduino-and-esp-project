#include <ESP8266WiFi.h>

#include <ESP8266WebServer.h>

#include <ESP8266mDNS.h>

#include <WiFiManager.h>

#include <DNSServer.h>

#include <EEPROM.h>

#include <ArduinoOTA.h>

#include <ESP8266HTTPClient.h>

ESP8266WebServer server(80);
String temperatureGet;

#define RELAYPIN D2
#define HOSTNAME "kombi"
#define TARGET_HOSTNAME "termostat.local"

bool isOpen;
IPAddress server_ip;
uint16_t port_number;
HTTPClient http;

bool resolve_mdns_service(char * service_name, char * protocol, char * desired_host, IPAddress * ip_addr, uint16_t * port_number) {
  Serial.println("Sending mDNS query");
  int n = MDNS.queryService(service_name, protocol, 10000);
  Serial.printf("mDNS query got %d results\n", n);

  if (n == 0) {
    Serial.println("no services found");
  } else {
    for (int i = 0; i < n; ++i) {
      if (strcmp(MDNS.hostname(i).c_str(), desired_host) == 0) {
        * ip_addr = MDNS.IP(i);
        * port_number = MDNS.port(i);
        return true;
      }
    }
  }
  return false;
}


int getTempServer() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    WiFiClient client;
    http.begin(client, server_ip.toString(), 80, "/TEMP");

    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        return http.getString().toInt();
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      return -255;
    }
    http.end();
  }
  delay(1000);
}


//Handles http request
void handleRoot() {
  server.send(200, "text/html", "<!DOCTYPE html> <html> <head> <style> .onof input{ padding: 10px; height: 100px; width: 100px; text-decoration: none; } #container form{ } </style> </head> <body> <center> <h2>Kombi Kontrol</h2> <br> <div id='container'> <form action='/TEMPSET'> Ayarlanacak Derece:<br> <input type='text' name='temp' placeholder='Sadece rakam giriniz!!'> <br><br> <input type='submit' value='Ayarla'> </form> <br> <form class='onof' action='/ON'><input type='submit' value='AC' /></form> <form class='onof' action='/OFF'><input type='submit' value='KAPAT' /></form> </div> </center> </body> </html> ");
}

void initPinOuts() {
  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, LOW);
}

void initWFM() {
  WiFiManager wifiManager;
  wifiManager.autoConnect("Kombi");
  Serial.println("connected... Network");
}

void handleON() {
  isOpen = true;
  server.send(200, "text/plain", "OK");
  Serial.println("Request ON");
}

void handleOFF() {
  isOpen = false;
  server.send(200, "text/plain", "OK");
  Serial.println("Request OFF");
}

void handleTempSet() {
  if (!server.hasArg("temp") ||
    server.arg("temp") == NULL) { // If the POST request doesn't have temp data
    server.send(400, "text/plain", "400: Invalid Request"); // The request is invalid, so send HTTP status 400
    return;
  } else {
    temperatureGet = server.arg("temp");
    byte tempcast = (byte) temperatureGet.toInt();
    EEPROM.write(1, tempcast);
    if (EEPROM.commit()) {
      server.send(200, "text/plain", "EEPROM successfully committed");
    } else {
      server.send(400, "text/plain", "ERROR! EEPROM commit failed");
    }
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  initWFM();
  initPinOuts();
  Serial.begin(115200);
  EEPROM.begin(1);
  //TempEqual.attach(60,isTempEqual);
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); //You can get IP address assigned to ESP

  if (WiFi.status() == WL_CONNECTED) //If WiFi connected to hot spot then start mDNS
  {
    if (MDNS.begin(HOSTNAME)) { //Start mDNS with name esp8266
      Serial.println("MDNS started");
    }
  }
  http.setReuse(true);
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

  server.on("/", HTTP_GET, handleRoot); //Associate handler function to path
  server.on("/ON", HTTP_GET, handleON);
  server.on("/OFF", HTTP_GET, handleOFF);
  server.on("/TEMPSET", handleTempSet);
  server.begin(); //Start server
  Serial.println("HTTP server started");
  MDNS.addService("http", "tcp", 80);



  if (resolve_mdns_service("http", "tcp", TARGET_HOSTNAME, & server_ip, & port_number)) {
    Serial.printf("got an answer for %s\n", TARGET_HOSTNAME);
    Serial.println(server_ip);
    Serial.println(port_number);
    isOpen = true;
  } else {
    Serial.printf("Sorry, %s not found\n", TARGET_HOSTNAME);
    isOpen = false;
  }
}


// the loop function runs over and over again forever
void loop() {
  if (isOpen) {
    byte temperature = EEPROM.read(1);
    int temperatureCE = getTempServer();
    if (temperatureCE = !-255) {
      if (temperatureCE < temperature) {
        digitalWrite(RELAYPIN, HIGH);
      } else {
        digitalWrite(RELAYPIN, LOW);
      }
    }
  } else {
    digitalWrite(RELAYPIN, LOW);
    if (resolve_mdns_service("http", "tcp", TARGET_HOSTNAME, & server_ip, & port_number)) {
      Serial.printf("got an answer for %s\n", TARGET_HOSTNAME);
      Serial.println(server_ip);
      Serial.println(port_number);
      isOpen = true;
    } else {
      Serial.printf("Sorry, %s not found\n", TARGET_HOSTNAME);
      isOpen = false;
    }
  }

  server.handleClient();
  MDNS.update();
  ArduinoOTA.handle();
}