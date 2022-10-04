#include <Arduino.h> 
#include <SocketIoClient.h>
#include <WiFiManager.h>  
#include <WiFiClient.h> 
#include <math.h> 
#include <ArduinoJson.h> 
const size_t settingData = JSON_OBJECT_SIZE(2048);
StaticJsonDocument<settingData> settingDocs;
const size_t temperData = JSON_OBJECT_SIZE(256);
StaticJsonDocument<temperData> temperDocs;
DynamicJsonDocument meshDocs(1024);  
 
WiFiManager wm;
SocketIoClient webSocket; 

String espID = ""; 
String ID = "";
const char* Identify = "";  
uint32_t messageTimestamp = 0;  
int oneMin = 0;
String phoneId = ""; 

String cmd = "";
String cmdTemp = "";
String sData = "";
String msData = "";  
int emitCnt = 0;
int temperStamp = 0;
int temperStampTemp = 0;
int temperCnt = 0;
bool temperWorking = false;
void controlCommand(const char* payload, size_t length);

//--working commands variable--
 

//--sensor pin layout--
const int  SenA = 39;
const int  SenB = 34;
const int  SenC = 35;  
const int  SenCurrent = 36;
//--wash motor layout--
const int  wOnOff = 14;
const int  wDirection = 12; 
//--other pump layout--
const int pump1 = 32;
const int pump2 = 23;
const int pump3 = 25;
const int pump4 = 26; 
const int pump5 = 27; 
//--solenoid
const int valve1 = 15;
const int valve2 = 2; 
//--others-- 
const int plug1 = 0;
const int plug2 = 4; 
const int plug3 = 5; 
const int plug4 = 18; 
const int plug5 = 19; 
/////---pin layout ends

//---measureing value---
bool s_val[10] = {false,false,false,false,false,false,false,false,false,false}; 
int current_val = 0;
float temper_current = 0;
float temperature = 0;

//---processing stage---
bool fPower = true;          // power of filter
int fCycleMode = 0;          //stage: water supply 1, cycle 2, clean 3, lense 4
int filterMode = 0;          //work mode : manual 1, auto 2, AI 3 
int cleanStep = 0;           //in clean stage...
int cleanTime = 0;
int serverTimecmd = 0;       //this time is the server time ex: 17*60+56  17:56
uint32_t timeStamp = 0;      //this is timestamp converted second ex:1629477685
uint32_t forceS = 0;         //force starting time with manual button press 
bool startFlag = false;
float eTemper = 0;
float sTemper = 0;
float tSpeed = 0;
bool tMode = false;
bool tPower = true;

//플러그의 전원유지상태 남은 시간 ...분
int p1V = 0;
int p2V = 0;
int p3V = 0;
int p4V = 0;
int p5V = 0;


#define RXD2 16
#define TXD2 17

void pinSet(){ 
   pinMode(SenA, INPUT);
   pinMode(SenB, INPUT);
   pinMode(SenC, INPUT); 
   pinMode(SenCurrent, INPUT); 
   pinMode(wOnOff, OUTPUT);
   pinMode(wDirection, OUTPUT);
   pinMode(pump1, OUTPUT);
   pinMode(pump2, OUTPUT);
   pinMode(pump3, OUTPUT);
   pinMode(pump4, OUTPUT);
   pinMode(pump5, OUTPUT);
   pinMode(valve1, OUTPUT);
   pinMode(valve2, OUTPUT);
   pinMode(plug1, OUTPUT);
   pinMode(plug2, OUTPUT);
   pinMode(plug3, OUTPUT);
   pinMode(plug4, OUTPUT);
   pinMode(plug5, OUTPUT);
   
   digitalWrite(wOnOff, LOW); 
   digitalWrite(wDirection, LOW); 
   digitalWrite(pump1, LOW); 
   digitalWrite(pump2, LOW); 
   digitalWrite(pump3, LOW);
   digitalWrite(pump4, LOW);
   digitalWrite(pump5, LOW);
   digitalWrite(plug1, LOW);
   digitalWrite(plug2, LOW);
   digitalWrite(plug3, LOW);
   digitalWrite(plug4, LOW);
   digitalWrite(plug5, LOW);
   digitalWrite(valve1, LOW);  //input solenoid
   digitalWrite(valve2, LOW);    
}

void setup() {
    pinSet();
    espID = "sii00" + WiFi.macAddress();
    ID = '\"' + espID + '\"';
    Identify = ID.c_str();     
    
    Serial.begin(115200);   
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    WiFi.mode(WIFI_STA);    
    if(wm.autoConnect("AutoConnectAP")){ 
         Serial.println("connected...yeey :)");             
         webSocket.begin("192.168.1.22", 8000);   
         webSocket.emit("new user",Identify );   
         webSocket.on("reply", messageEventHandler);
         webSocket.on("app-esp", controlCommand);       
    }  

}

void controlSolenoid(){
  if(s_val[6] == true){
    digitalWrite(valve1, HIGH);
  }
  if(s_val[5] == false){
    digitalWrite(valve1, LOW);
  }  
}

void supplyWater(){
   if(s_val[9] == true){
     digitalWrite(pump5, LOW);
     //alert alarm
   }else{
     digitalWrite(pump5, HIGH);
   }   
   controlSolenoid(); 
}

void cycleWater(){
  if(s_val[1] == false){
    digitalWrite(pump4, LOW);
    digitalWrite(pump1, HIGH);  
  }
  if(s_val[4] == true){
    digitalWrite(pump4, HIGH);  
    digitalWrite(pump1, LOW);
  }  
  controlSolenoid();
}

void cleanWater(){    
    if(cleanStep == 1){
      digitalWrite(pump1, LOW);  
    }
    if(cleanTime == 180){
      cleanStep = 2;   
    }
    if(cleanStep == 2){
      digitalWrite(pump2, HIGH);
      if(s_val[7] == true){
        digitalWrite(pump1, LOW);
        cleanStep = 3;
        cleanTime = 0;
      }
    }
    if(cleanStep == 3){
       int remain = cleanTime % 20;
       if(remain>=1 && remain <=8){
       
       }else if(remain>=11 && remain<=18){
        
       }else{
       
       }
       if(cleanTime > 900){
         cleanStep = 4;
       }
    }
    if(cleanStep == 4){
      digitalWrite(pump2, HIGH);
      if(s_val[8] == true){
        digitalWrite(pump2, LOW);
        cleanStep = 5;
      }  
    }  
    if(cleanStep == 5){
      digitalWrite(pump3, HIGH);
      if(s_val[7] == false){
        digitalWrite(pump3, LOW); 
        fCycleMode = 0; 
      }
      if(s_val[10] == true){
        digitalWrite(pump3, LOW);  
        fCycleMode = 0;
      }
    }
    controlSolenoid(); 
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
} 
 
void controlCommand(const char* payload, size_t length){  
   emitCnt--;
   //Serial.println("**************************"); 
   String Str = (String)payload;  
   //Serial.println(Str);
   char * cmd  = new char[Str.length() + 1];
     for(int i = 0; i < Str.length(); i++){
       cmd[i] = Str[i];
     }   
     deserializeJson(settingDocs, cmd);
     JsonObject settingObj = settingDocs.as<JsonObject>();
     Serial.println("##########"); 
     String phoneid = settingObj["phoneId"]; 
     bool fpower = settingObj["fPower"];
     int fcyclemode = settingObj["fCycleMode"];
     int filtermode = settingObj["filterMode"];
     float etemper = settingObj["eTemper"];
     float stemper = settingObj["sTemper"];
     float tspeed = settingObj["tSpeed"];
     bool tmode = settingObj["tMode"];
     bool tpower = settingObj["tPower"];

     String obj = settingObj["plug"]; 
     String p1 = getValue(obj,',',0);
     String p2 = getValue(obj,',',1);
     String p3 = getValue(obj,',',2);
     String p4 = getValue(obj,',',3);
     String p5 = getValue(obj,',',4);

   


     p1V = p1.toInt();
     p2V = p2.toInt();
     p3V = p3.toInt();
     p4V = p4.toInt();
     p5V = p5.toInt(); 
          
     phoneId = phoneid;
     fPower = fpower;
     fCycleMode = fcyclemode;
     filterMode = filtermode;
     eTemper = etemper;
     sTemper = stemper;
     tSpeed = tspeed;
     tMode = tmode;
     tPower = tpower;  
        
     meshDocs["et"] = eTemper;                      
     meshDocs["st"] = sTemper;           
     meshDocs["ts"] = tSpeed;
     meshDocs["tm"] = tMode;
     meshDocs["tp"] = tPower;   
     String msg;
     serializeJson(meshDocs, msg);  
     cmdWrite(msg); 
     
     String pl = settingObj["plug"];
     

   delete cmd;  
}

void messageEventHandler(const char* payload, size_t length){
     Serial.println("init");  
}  

//센서자료 측정 1초에 한번
int muxCnt = 0;
int muxA = 0;
int muxB = 0;
int muxC = 0;
void muxSnData(){
  //muxCnt++;
  delay(10);
  int sa = analogRead(SenA);
  delay(10);
  int sb = analogRead(SenB);
  delay(10);
  int sc = analogRead(SenC);
  //muxA = muxA + sa;
  //muxB = muxB + sb;
  //muxC = muxC + sc;
//  if(muxCnt == 10){
//    sa = floor(muxA/10);
//    sb = floor(muxB/10);
//    sc = floor(muxC/10);
//    muxA = 0;
//    muxB = 0;
//    muxC = 0;
//    muxCnt = 0;
//    Serial.println("sensing data");
//    Serial.println(sa);
//    Serial.println(sb);
//    Serial.println(sc);  
//  }
    Serial.println("sensing data");
    Serial.println(sa);
    Serial.println(sb);
    Serial.println(sc);  
  
   
}  

void cmdWrite(String msg){
  String sdata = (String)msg;
  for(int i = 0 ; i < sdata.length(); i++ ){  
     Serial2.write(sdata[i]);
  }     
}

//플러그동작시간 거꿀시계
void minCounter(){
  oneMin++;
  if(oneMin == 60){
    oneMin = 0; 
    if(p1V > 0){
      p1V--;
    } 
    if(p2V > 0){
      p2V--;
    } 
    if(p3V > 0){
      p3V--;
    } 
    if(p4V > 0){
      p4V--;
    } 
    if(p5V > 0){
      p5V--;
    } 
  } 
}

//설정값(남은시간)에 따르는 플러그 스위치
void controlPlug(){
   if(p1V > 0){
      digitalWrite(plug1, HIGH);
   }else{
      digitalWrite(plug1, LOW);
   }
   if(p2V > 0){
      digitalWrite(plug2, HIGH);
   }else{
      digitalWrite(plug2, LOW);
   }
   if(p3V > 0){
      digitalWrite(plug3, HIGH);
   }else{
      digitalWrite(plug3, LOW);
   }
   if(p4V > 0){
      digitalWrite(plug4, HIGH);
   }else{
      digitalWrite(plug4, LOW);
   }
   if(p5V > 0){
      digitalWrite(plug5, HIGH);
   }else{
      digitalWrite(plug5, LOW);
   }
}

void cmdAnalysis(String payload){ 
   //Serial.println(payload);
   String Str = payload;  
   char * cmd  = new char[Str.length() + 1];
     for(int i = 0; i < Str.length(); i++){
       cmd[i] = Str[i];
     }   
     deserializeJson(temperDocs, cmd);
     JsonObject tempObj = temperDocs.as<JsonObject>();
    // Serial.println("##########"); 
     float cu = tempObj["CU"]; 
     float te = tempObj["TE"];
     String id = tempObj["ID"];
     int st = tempObj["ST"];
     temperStamp = st;
     temper_current =  floor(cu*10)/10;
     temperature =  floor(te*10)/10;
   delete cmd;        
} 

void controlFilter(){
  controlSolenoid();
  //filterMode;
  if(fCycleMode == 1){
     supplyWater();
  }else if(fCycleMode == 2){
    supplyWater();
    cycleWater();
  }else if(fCycleMode == 3){
    cleanWater();
  }else if(fCycleMode == 4){
    cleanWater();
  }else{
  
  }
}

float currentRes;
int readValue;
int maxValue = 0;
int minValue = 1024;

void emitData(){ 
  
   String sState = String(s_val[0])+String(s_val[1])+String(s_val[2])+String(s_val[3])+String(s_val[4]);
   String myData = "{'temperature':"+String(temperature)+ ",'tw':" + String(temperWorking) + ", 'tCurrent':"+String(temper_current)+ ", 'cycleSeq':"+String(fCycleMode)+ ", 'sState':'"+ sState + "', 'current':"+String(current_val)+ ", 'tc':"+String(99)+ ", 'tc':"+String(99)+ "}";
   String muxCmd = "{'id':'" + espID + "','pid':'" + phoneId + "','data':"+ myData + ",'ts':'" + timeStamp + "'}" ;
   sData =    '\"' + muxCmd + '\"';   
   const char* cstr = sData.c_str();
   if(webSocket.connect_state =="discon"){
      webSocket.disconnect(); 
      webSocket.emit("new user",Identify );
   }
   if(webSocket.connect_state =="con"){ 
    if(cmdTemp != muxCmd){
      cmdTemp = muxCmd;
      emitCnt++;
      webSocket.emit("esp message",cstr ); 
    }     
   }  
} 

void loop() {    
  
      uint32_t now = millis();  
      
      if(abs(now - messageTimestamp) > 1000){   
         messageTimestamp = now;    
         muxSnData();  
      }   
    
}
