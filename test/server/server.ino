#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h> 

DynamicJsonDocument meshDocs(1024);  
StaticJsonDocument<250> jsonDocument;
/* Put your SSID & Password */
const char* ssid = "NodeMCU";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);

void setup() {
   Serial.begin(115200);
   WiFi.softAP(ssid, password);
   WiFi.softAPConfig(local_ip, gateway, subnet);
   delay(100); 

   server.on("/", handle_OnConnect);
   server.on("/sensing", HTTP_POST, handle_Sensing);

   server.onNotFound(handle_NotFound);
  
   server.begin();
   Serial.println("HTTP server started");
}

void handle_OnConnect() { 
  Serial.println("client connected");
     meshDocs["et"] = 12;                      
     meshDocs["st"] = 14;           
     meshDocs["ts"] = 16;
     String msg;
     serializeJson(meshDocs, msg);  
  server.send(200, "text/plain", msg); 
}

void handle_Sensing(){
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);
  int red = jsonDocument["red"];
  int green = jsonDocument["green"];
  int blue = jsonDocument["blue"];
  Serial.println("----------");
  Serial.println(red);
  Serial.println(green);

     meshDocs["et"] = 12;                      
     meshDocs["st"] = 14;           
     meshDocs["ts"] = 16;
     String msg;
     serializeJson(meshDocs, msg);  
     
  server.send(200, "application/json", msg);
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void loop() {
  server.handleClient();

}
