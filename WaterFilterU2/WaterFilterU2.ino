#include <WiFi.h>
#include <WebServer.h>
#include <SoftwareSerial.h> 
#include <ArduinoJson.h>  
#include <TaskScheduler.h> 
#include <SoftwareSerial.h> 

DynamicJsonDocument meshDocs(512);  
StaticJsonDocument<512> jsonDocument;

const char* ssid = "NodeMCU";  
const char* password = "12345678";  
IPAddress local_ip(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);
WebServer server(80);  
bool button1_status = 0;   
uint32_t messageTimestamp = 0;  
 

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
     String msg = "welcome song";
     serializeJson(meshDocs, msg);  
     server.send(200, "text/plain", msg); 
}

void handle_Sensing(){
    Serial.println("get http request"); 
    server.send(200, "text/plain", "ghost"); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

 



void loop() {  
    server.handleClient();   
    uint32_t now = millis();  
    if(abs(now - messageTimestamp) > 2000){   
       messageTimestamp = now;    
       Serial.println("working...");
    }   
}
