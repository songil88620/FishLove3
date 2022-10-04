#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h> 
#include <SoftwareSerial.h> 
#include <ArduinoJson.h> 
 

DynamicJsonDocument meshDocs(256);  
StaticJsonDocument<256> jsonDocument;

const char* ssid = "NodeMCU";  
const char* password = "12345678";  
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0); 
ESP8266WebServer server(80); 

bool button1_status = 0;
#define Button1 D5 

String senseData = "";
String cmdData = ""; 

void setup() {
    Serial.begin(115200); 
    pinMode(Button1, INPUT); 
    pinMode(LED_BUILTIN, OUTPUT);

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
    String sdata;
    String body = server.arg("plain");
    sdata = body.c_str();  
    Serial.println(sdata); 
    for(int i = 0 ; i < sdata.length(); i++ ){  
       innercom.write(sdata[i]);
    }                                                                                   
    delay(10);   
    server.send(200, "text/plain", cmdData);
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void cmdAnalysis(String cmdMsg){  
   cmdData = cmdMsg;
}

void loop() { 
    server.handleClient();
    
    String cmdTemp = ""; 
    bool cmdFlag = false;
     
    if(cmdFlag){
      Serial.println("통보문받기끝...");
      Serial.println(cmdTemp);   
      cmdFlag = false; 
      cmdAnalysis(cmdTemp);        //명령해석 및 처리...      
    }

}
