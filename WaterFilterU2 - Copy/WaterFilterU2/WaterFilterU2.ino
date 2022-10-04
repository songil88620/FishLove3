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

Scheduler ts;
void getVPP();
Task t1 (4000 * TASK_MILLISECOND, TASK_FOREVER, &getVPP, &ts, true);

bool button1_status = 0;
#define RXD2 19
#define TXD2 18
SoftwareSerial innercom(RXD2,TXD2);

const int cs10 = 27;
const int cs11 = 32;
const int cs12 = 33;
const int cs20 = 14;
const int cs21 = 25;
const int cs22 = 26;
const int sen1 = 34;
const int sen2 = 35; 
const int led1 = 5;
const int led2 = 12;

String senseData = "";
String cmdData = "";
float currentMux[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
int maxMux[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
int minMux[13] = {1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024};
int readMux[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0}; 
String temperData = "none";

uint32_t messageTimestamp = 0; 

float senCu(int index, int cs0, int cs1, int cs2){
   float res = 0.0;
   if(index == 1){
      digitalWrite(cs10, cs0); 
      digitalWrite(cs11, cs1); 
      digitalWrite(cs12, cs2); 
      delay(5);
      res = analogRead(sen1);
   }else{
      digitalWrite(cs20, cs0); 
      digitalWrite(cs21, cs1);
      digitalWrite(cs22, cs2);
      delay(5);
      res = analogRead(sen2);
   }
   return res;
}

void pinInit(){
   digitalWrite(cs10, LOW); 
   digitalWrite(cs11, LOW); 
   digitalWrite(cs12, LOW); 
   digitalWrite(cs20, LOW); 
   digitalWrite(cs21, LOW);
   digitalWrite(cs22, LOW); 
   digitalWrite(led1, LOW);
   digitalWrite(led2, LOW); 
}
void pinModeSet(){
   pinMode(cs10, OUTPUT); 
   pinMode(cs11, OUTPUT); 
   pinMode(cs12, OUTPUT); 
   pinMode(cs20, OUTPUT); 
   pinMode(cs21, OUTPUT); 
   pinMode(cs22, OUTPUT); 
   pinMode(led1, OUTPUT); 
   pinMode(led2, OUTPUT); 
}

void setup() {
    Serial.begin(115200);
    pinModeSet();
    pinInit();
    //Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);
    innercom.begin(19200);
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

void handle_upload(){ 
    String bodyCurrent = "";
    for(int i = 0; i < 13; i++){
      bodyCurrent += String(currentMux[i])+",";
    } 
    int removeAt = bodyCurrent.length() - 1;
    bodyCurrent.remove(removeAt);
    bodyCurrent = bodyCurrent + "`" + temperData;

    Serial.println("sensing data");
    Serial.println(temperData);
    Serial.println(bodyCurrent);
    String ss = "ghost";
    String sdata = bodyCurrent.c_str();
    
    Serial.println(sdata);
    for(int i = 0 ; i < sdata.length(); i++ ){
       innercom.write(sdata[i]);
    }      
}

void handle_Sensing(){
    Serial.println("get http request");
    temperData =String(server.arg("plain"));
    server.send(200, "text/plain", cmdData); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void cmdAnalysis(String cmdMsg){  
   cmdData = cmdMsg;
}


void getVPP(){      
  
  memset(currentMux, 0, sizeof(currentMux));
  memset(readMux, 0, sizeof(readMux));
  memset(maxMux, 0, sizeof(maxMux));
  memset(minMux, 1024, sizeof(minMux));
  int cntCheck = 50;
   uint32_t start_time = millis();
   while(cntCheck>0){
       cntCheck--; 
       readMux[0] += senCu(1, 0, 0, 0); 
       readMux[1] += senCu(1, 1, 0, 0);  
       readMux[2] += senCu(1, 0, 1, 0); 
       readMux[3] += senCu(1, 1, 1, 0); 
       readMux[4] += senCu(1, 0, 0, 1); 
       readMux[5] += senCu(1, 1, 0, 1); 
       readMux[6] += senCu(1, 0, 1, 1); 
       readMux[7] += senCu(1, 1, 1, 1); 
       readMux[8] += senCu(2, 0, 0, 0); 
       readMux[9] += senCu(2, 1, 0, 0); 
       readMux[10] += senCu(2, 0, 1, 0); 
       readMux[11] += senCu(2, 1, 1, 0); 
       readMux[12] += senCu(2, 0, 0, 1); 
   }
   cntCheck = 50;
   for(int i = 0; i<13; i++){ 
     readMux[i] = readMux[i] / cntCheck;
     Serial.print("-------:");
     Serial.println(readMux[i]);
   }
   for(int i=0;i<13;i++){
    currentMux[i] = (( readMux[i] * (3.3/4095.0)) - 2.3 )/0.1; 
   }
}

void loop() { 
    ts.execute();
    server.handleClient(); 
    String cmdTemp = ""; 
    bool cmdFlag = false;
    while(innercom.available()){   //centerO로 부터 명령대가중...
      cmdTemp = innercom.readString(); 
      cmdFlag = true;
    }
    if(cmdFlag){
      Serial.println("통보문받기끝...");
      Serial.println(cmdTemp);   
      cmdFlag = false; 
      cmdAnalysis(cmdTemp);        //명령해석 및 처리...      
    }
    uint32_t now = millis();  
    if(abs(now - messageTimestamp) > 2000){   
       messageTimestamp = now;    
       handle_upload();        
    }   
}
