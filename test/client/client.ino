#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h> 

DynamicJsonDocument meshDocs(1024); 
StaticJsonDocument<250> jsonDocument;
const char* ssid = "NodeMCU";
const char* password = "12345678";  
const char* serverName = "http://192.168.1.1:80/sensing";

unsigned long lastTime = 0; 
unsigned long timerDelay = 1000; 

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

}

void loop() {
   if ((millis() - lastTime) > timerDelay) {
    Serial.println("here");
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http; 
      http.begin(client, serverName); 
   
      http.addHeader("Content-Type", "text/plain"); 

      meshDocs["red"] = 12;                      
      meshDocs["green"] = 14;           
      meshDocs["blue"] = 16;
      String httpRequestData;
      serializeJson(meshDocs, httpRequestData);  
      int httpResponseCode = http.POST(httpRequestData);   
      delay(50);
      String payload = http.getString();
      Serial.print("HTTP Response code: ");
      Serial.println(payload);

        deserializeJson(jsonDocument, payload);
        int et = jsonDocument["et"];
        int st = jsonDocument["st"];
        int ts = jsonDocument["ts"];
        Serial.println(et);
        Serial.println(st);
        Serial.println(ts);
      
      Serial.println(httpResponseCode);
        
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

}
