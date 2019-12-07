#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>

#define TARGET_HOSTNAME "esp8266.local"

IPAddress server_ip;
uint16_t port_number;
HTTPClient http;
bool isOpen;

int getTempServer(){
 if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
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


bool resolve_mdns_service(char* service_name, char* protocol, char* desired_host, IPAddress* ip_addr, uint16_t *port_number) {
  Serial.println("Sending mDNS query");
  int n = MDNS.queryService(service_name, protocol);
  Serial.printf("mDNS query got %d results\n", n);

  if(n == 0) {
    Serial.println("no services found");
  } else {
    for (int i = 0; i < n; ++i) {
      if(strcmp(MDNS.hostname(i).c_str(), desired_host) == 0) {
        *ip_addr = MDNS.IP(i);
        *port_number = MDNS.port(i);
        return true;
      }
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin("Sona", "Ar102797");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  http.setReuse(true);
 if (WiFi.status() == WL_CONNECTED) //If WiFi connected to hot spot then start mDNS
  {
    if (MDNS.begin("tempgetter")) {  //Start mDNS with name esp8266
      Serial.println("MDNS started");
    }
  }
  MDNS.addService("http", "tcp", 80);
  if(resolve_mdns_service("http", "tcp", TARGET_HOSTNAME, &server_ip, &port_number)) {
    Serial.printf("got an answer for %s\n", TARGET_HOSTNAME);
    Serial.println(server_ip);
    Serial.println(port_number);
    isOpen = true;
  } else {
    Serial.printf("Sorry, %s not found\n", TARGET_HOSTNAME);
    isOpen = false;
  }
}

void loop() {
  if(isOpen){
  getTempServer();
  delay(4000);
  }
}
