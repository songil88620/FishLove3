#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h> 
#include <DallasTemperature.h>
#include <OneWire.h>
#include <TaskScheduler.h> 
#define ONE_WIRE_BUS 14        //D5 pin of nodemcu
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);    
#define RELAY_PIN 12  
Scheduler ts;
void task1Callback();
Task t1 (1000 * TASK_MILLISECOND, TASK_FOREVER, &task1Callback, &ts, true);

double nCurrent; 
const int sensorIn = A0;
int mVperAmp = 100; // use 100 for 20A Module and 66 for 30A Module 
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;   

double present_tmp = 0.0;  
double target_tmp = 0.0;
float startTemp = 0.0;
float speedTemp = 0.0;
bool modeTemp = false;
bool powerTemp = true;

DynamicJsonDocument meshDocs(1024); 
StaticJsonDocument<250> jsonDocument;
const char* ssid = "NodeMCU";
const char* password = "12345678";  
const char* serverName = "http://192.168.1.1:80/sensing";

unsigned long lastTime = 0; 
unsigned long timerDelay = 1000; 

 
WiFiClient client;
HTTPClient http;  

void task1Callback() {
   Voltage = getVPP();
   VRMS = (Voltage/2.0) *0.707;  //root 2 is 0.707
   AmpsRMS = (VRMS * 1000)/mVperAmp;     
   nCurrent = AmpsRMS;  
   Serial.println("전류측정함...");
   Serial.println(nCurrent); 

}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT); 
  sensors.begin();
  pinMode(A0, INPUT); 
  //mycurrent.attach(1, measureCurrent); 
 
}



float getVPP(){
  float result;
  int readValue;         
  int maxValue = 0;          
  int minValue = 1024;          
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000){          
       readValue = analogRead(sensorIn);   
       if (readValue > maxValue){ 
           maxValue = readValue;
       }
       if (readValue < minValue){
           minValue = readValue;
       }
   } 
   result = ((maxValue - minValue) * 5.0)/1024.0; 
   return result;
}

void loop() {
   ts.execute();
   if ((millis() - lastTime) > timerDelay) {
    Serial.println("here");
    
    if(WiFi.status()== WL_CONNECTED){  
      http.begin(client, serverName);  
      http.addHeader("Content-Type", "text/plain");  
      meshDocs["ID"] = "temperature";                      
      meshDocs["CU"] = nCurrent;   
      meshDocs["ST"] = millis();
      sensors.requestTemperatures();          
      meshDocs["TE"] = sensors.getTempCByIndex(0);  
      Serial.println("Temperature is: ");       
      Serial.println(sensors.getTempCByIndex(0));  
      present_tmp = sensors.getTempCByIndex(0);    
      String httpRequestData;
      serializeJson(meshDocs, httpRequestData);  
      int httpResponseCode = http.POST(httpRequestData);   
      delay(10);
      
      String payload = http.getString();
      Serial.print("HTTP Response code: ");
      Serial.println(payload);

        deserializeJson(jsonDocument, payload);
        float et = jsonDocument["et"];
        float st = jsonDocument["st"];
        float ts = jsonDocument["ts"];
        bool tm = jsonDocument["tm"];
        bool tp = jsonDocument["tp"];
        target_tmp = et;
        startTemp = st;
        speedTemp = ts;
        modeTemp = tm;
        powerTemp = tp; 
                 
        Serial.println(et);
        Serial.println(st);
        Serial.println(ts);    
          
      Serial.println(httpResponseCode);  
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

  if(target_tmp > present_tmp){  
    if(powerTemp == true){
      digitalWrite(RELAY_PIN, HIGH);
    }else{
      digitalWrite(RELAY_PIN, LOW);
    }     
  }
  else{        
    digitalWrite(RELAY_PIN, LOW);
  } 

}
