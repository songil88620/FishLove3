 
#include <SoftwareSerial.h> 
#include <ArduinoJson.h>  
#include <TaskScheduler.h> 
#include <SoftwareSerial.h> 
#include <DallasTemperature.h>
#include <OneWire.h> 
const int ONE_WIRE_BUS = 15;
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire); 

DynamicJsonDocument meshDocs(512);  
StaticJsonDocument<512> jsonDocument;
Scheduler ts;
void getVPP();
Task t1 (3000 * TASK_MILLISECOND, TASK_FOREVER, &getVPP, &ts, true);

bool button1_status = 0;
#define RXD2 19
#define TXD2 18
SoftwareSerial innercom(RXD2,TXD2);

const int sen1 = 36;
const int sen2 = 39;
const int sen3 = 34;
const int sen4 = 35;
const int st1 = 5;
const int st2 = 12; 

String senseData = "";
String cmdData = "";
float currentMux[4] = {0,0,0,0};  
String temperData = "none";

uint32_t messageTimestamp = 0;  

void pinInit(){ 
   digitalWrite(st1, LOW);
   digitalWrite(st2, LOW); 
}
void pinModeSet(){
   pinMode(sen1, INPUT); 
   pinMode(sen2, INPUT); 
   pinMode(sen3, INPUT); 
   pinMode(sen4, INPUT);  
   pinMode(st1, INPUT);
   pinMode(st2, INPUT);
}

void setup() {
    Serial.begin(115200);
    pinModeSet();
    pinInit(); 
    innercom.begin(19200);  
    sensors.begin();
} 

void handle_upload(){ 
    sensors.requestTemperatures();    
    double present_tmp = sensors.getTempCByIndex(0);
    Serial.println(">>>>>>>TEMP");
    Serial.println(String(present_tmp));
    String bodyCurrent = "";
    for(int i = 0; i < 4; i++){
      bodyCurrent += String(currentMux[i])+",";
    } 
    int removeAt = bodyCurrent.length() - 1;
    bodyCurrent.remove(removeAt);
    bodyCurrent = bodyCurrent + "`" + String(present_tmp);

    Serial.println("sensing data");
    Serial.println(present_tmp);
    Serial.println(bodyCurrent); 
    
    String sdata = bodyCurrent.c_str();  
    for(int i = 0 ; i < sdata.length(); i++ ){
       innercom.write(sdata[i]);
    }      
}

void getVPP(){  
  memset(currentMux, 0, sizeof(currentMux));
  for(int k = 0; k<4; k++){
    float result;
    int readValue;         
    int maxValue = 0;          
    int minValue = 4095;          
    
     uint32_t start_time = millis();
     while((millis()-start_time) < 1000){   
         if(k == 0){
            readValue = analogRead(sen1);   
         }else if(k == 1){
            readValue = analogRead(sen2);   
         }else if(k == 2){
            readValue = analogRead(sen3);   
         }else{
            readValue = analogRead(sen4);   
         }        
         if (readValue > maxValue){ 
             maxValue = readValue;
         }
         if (readValue < minValue){
             minValue = readValue;
         }
     } 
     result = ((maxValue - minValue) * 3.3)/4095.0; 
     currentMux[k] = (result/2.0)*0.707*1000/100;   
     Serial.print(">>>>>Current");
     Serial.println(String(result));     
  } 
}

void loop() { 
    ts.execute();  
    uint32_t now = millis();  
    if(abs(now - messageTimestamp) > 1000){   
       messageTimestamp = now;    
       handle_upload();        
    }   
}
