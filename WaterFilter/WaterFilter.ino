#include <Arduino.h> 
#include <SocketIoClient.h>
#include <WiFiManager.h>  
#include <WiFiClient.h> 
#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h> 
#include <ArduinoJson.h> 
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
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
void messageEventHandler(const char* payload, size_t length);
//--working commands variable--
 

//--sensor pin layout--
const int  SenA = 39;
const int  SenB = 34;
const int  SenC = 35;  
//--wash motor layout--
const int  wOnOff = 12;
const int  wDirection = 14; 
//--other pump layout--
const int pump1 = 32;
const int pump2 = 33;
const int pump3 = 25;
const int pump4 = 26; 
const int pump5 = 27; 
//--solenoid
const int valve1 = 15; 
//--others-- 
const int plug1 = 2;
const int plug2 = 4; 
const int plug3 = 5; 
const int plug4 = 18; 
const int plug5 = 19;
const int plug6 = 13;
const int swLed = 21; 
/////---pin layout ends

//---measureing value---
bool s_val[10] = {false,false,false,false,false,false,false,false,false,false}; 
bool s_state[14] = {false,false,false,false,false,false,false,false,false,false,false,false,false,false};
int current_val = 0;
float temper_current = 0;
float temperature = 0;
String c_val = ""; // current value mux

//---processing stage---
bool fPower = true;          // power of filter
int fCycleMode = 0;          //stage: water supply 1, cycle 2, clean 3, lense 4
int fCycleModeTemp = 0;
int filterMode = 0;          //work mode : manual 1, auto 2, AI 3 
int cleanStep = 1;           //in clean stage...
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

void pinInit(){
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
   digitalWrite(plug6, LOW);
   digitalWrite(valve1, LOW);  //input solenoid
   memset(s_state, false, sizeof(s_state));
}
void pinSet(){ 
   pinMode(SenA, INPUT);
   pinMode(SenB, INPUT);
   pinMode(SenC, INPUT); 
   pinMode(wOnOff, OUTPUT);
   pinMode(wDirection, OUTPUT);
   pinMode(pump1, OUTPUT);
   pinMode(pump2, OUTPUT);
   pinMode(pump3, OUTPUT);
   pinMode(pump4, OUTPUT);
   pinMode(pump5, OUTPUT);
   pinMode(valve1, OUTPUT); 
   pinMode(plug1, OUTPUT);
   pinMode(plug2, OUTPUT);
   pinMode(plug3, OUTPUT);
   pinMode(plug4, OUTPUT);
   pinMode(plug5, OUTPUT);
   pinMode(plug6, OUTPUT);
   pinInit();
}

void setup() {
    // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable   detector
    pinSet();
    espID = "sii00" + WiFi.macAddress();
    ID = '\"' + espID + '\"';
    Identify = ID.c_str();     
    
    Serial.begin(115200);   
    Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);
    WiFi.mode(WIFI_STA);    
    if(wm.autoConnect("AutoConnectAP")){ 
         Serial.println("connected...yeey :)");             
         webSocket.begin("192.168.1.28", 8000);   
         webSocket.emit("new user",Identify );   
         webSocket.on("reply", messageEventHandler);
         webSocket.on("app-esp", controlCommand);       
    } 
    muxSnData();
    initialData(); 
    Serial.println("setup end...");
    delay(1000);
}


const char* apiServer = "http://192.168.1.28:8000/init_data";
void initialData(){ 
  String sensorReadings = httpGETRequest(apiServer);
  Serial.println("API data");
  Serial.println(sensorReadings);
  const char* payload = sensorReadings.c_str(); 
  controlCommand(payload, 0);
//  JSONVar myObject = JSON.parse(sensorReadings);
//  if (JSON.typeof(myObject) == "undefined") {
//    Serial.println("Parsing input failed!");
//    return;
//  } 
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName); 
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  espID = "sii00" + WiFi.macAddress();
  String httpRequestData = "id=" + espID;  
  int httpResponseCode = http.POST(httpRequestData);
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end(); 
  return payload;
}

void controlSolenoid(){
  if(s_val[6] == true){
    digitalWrite(valve1, HIGH);
    s_state[6] = true;
    Serial.println("쏠레노이드 열기");
  }
  if(s_val[5] == false){
    digitalWrite(valve1, LOW);
    s_state[6] = false;
    Serial.println("쏠레노이드 닫기");
  }  
}  
void supplyWater(){
   if(s_val[9] == true){
     digitalWrite(pump5, LOW);
     s_state[5] = false;
     Serial.println("펌프5 닫기");
     //alert alarm
   }else{
     digitalWrite(pump5, HIGH);
     s_state[5] = true;
     Serial.println("펌프5 열기");
   }  
}

bool cycleInitial = true;

void cycleWater(){
  if(cycleInitial){
    cycleInitial = false;
    digitalWrite(pump4, HIGH);
    s_state[4] = true;
  }
  if(s_val[1] == false){
    digitalWrite(pump1, HIGH); 
    digitalWrite(pump4, LOW);
    s_state[4] = false;
    s_state[1] = true;     
  }
  if(s_val[4] == true){
    digitalWrite(pump1, LOW);
    digitalWrite(pump4, HIGH); 
    s_state[4] = true;
    s_state[1] = false;       
  }   
}

bool nowWater = false;

void cleanWater(){    
    if(s_val[4] == true){         
      digitalWrite(pump4, HIGH);   
      s_state[4] = true;
    }
    if(s_val[1] == false){         
      digitalWrite(pump4, LOW); 
      s_state[4] = false;    
    }  
    nowWater = true;
    cleanTime++;
    Serial.print("clenaTime:");
    Serial.println(cleanTime);
    Serial.println(cleanStep);
    if(cleanStep == 1){
      digitalWrite(pump1, LOW); 
      s_state[1] = false;   
    }
    if(cleanTime == 60 && cleanStep == 1){
      cleanStep = 2;   
    }
    if(cleanStep == 2){
      Serial.println("2번펌프열기");
      digitalWrite(pump2, HIGH);
      s_state[2] = true;   
      if(s_val[7] == true){
        digitalWrite(pump2, LOW);
        s_state[2] = false;
        Serial.println("2번펌프닫기");
        cleanStep = 3;
        cleanTime = 0;
      }
    }
    if(cleanStep == 3){
       int remain = cleanTime % 20;
       if(remain>=1 && remain <=8){ 
         Serial.println("정방향");
         digitalWrite(wDirection, HIGH);
         digitalWrite(wOnOff, HIGH);
         s_state[0] = true;
         s_state[13] = true;
         delay(10);
       }else if(remain>=11 && remain<=18){
         digitalWrite(wDirection, LOW);
         Serial.println("역정향");
         digitalWrite(wOnOff, HIGH);
         s_state[0] = true;
         s_state[13] = false;
         delay(10);
       }else{
         Serial.println("정자");
         digitalWrite(wOnOff, LOW);
         s_state[0] = false; 
       }
       if(cleanTime > 60){
         cleanStep = 4;
         cleanTime = 0;
         digitalWrite(wOnOff, LOW);
       }
    }
    if(cleanStep == 4){
      digitalWrite(pump2, HIGH);
      s_state[2] = true; 
      Serial.println("2번펌프열기");
      if(s_val[8] == true){
        digitalWrite(pump2, LOW);
        s_state[2] = false; 
        Serial.println("2번펌프닫기");
        cleanStep = 5;
      }  
    }  
    if(cleanStep == 5){
      if(s_val[7] == true){
       digitalWrite(pump3, HIGH);
       s_state[3] = true; 
       Serial.println("3번펌프열기");
      }      
      if(s_val[7] == false){
        digitalWrite(pump3, LOW); 
        s_state[3] = false; 
        Serial.println("3번펌프닫기");
        //clean end... 
        cleanTime = 0; 
        nowWater = false;
        cleanStep = 1;
        String cid = "gLinse-" + String(fCycleMode);  
        emitData(cid, fCycleMode); 
        initPump();
        delay(100);
        initialData();
        if(filterMode == 2 || filterMode == 3){
          fCycleMode = 4;
        }else{
          fCycleMode = 2;
        }
      }
      if(s_val[0] == true){
        digitalWrite(pump3, LOW);  
        s_state[3] = false;  
      } 
    } 
}

void linseWater() {
    if(s_val[4] == true){         
      digitalWrite(pump4, HIGH); 
      s_state[4] = true;     
    }
    if(s_val[1] == false){         
      digitalWrite(pump4, LOW); 
      s_state[4] = false;     
    }
    cleanTime++;
    nowWater = true;
    Serial.print("clenaTime:");
    Serial.println(cleanTime);
    if(cleanStep == 1){
      digitalWrite(pump1, LOW);  
      s_state[1] = false; 
    }
    if(cleanTime == 60 && cleanStep == 1){
      cleanStep = 2;
    }
    if(cleanStep == 2){
      digitalWrite(pump2, HIGH);
      s_state[2] = true; 
      if(s_val[7] == true){
        digitalWrite(pump2, LOW);
        s_state[2] = false; 
        cleanStep = 3;
        cleanTime = 0;
      }
    }
    if(cleanStep == 3){
       int remain = cleanTime % 20;
       if(remain>=1 && remain <=8){ 
         digitalWrite(wDirection, HIGH);
         digitalWrite(wOnOff, HIGH);
         s_state[13] = true;
         s_state[0] = true;
         delay(10);
       }else if(remain>=11 && remain<=18){
         digitalWrite(wDirection, LOW);
         digitalWrite(wOnOff, HIGH);
         s_state[13] = false;
         s_state[0] = true;
         delay(10);
       }else{
         digitalWrite(wOnOff, LOW);
         s_state[0] = false;
       }
       //if(cleanTime > 900){
       if(cleanTime > 60){
         cleanStep = 4;
         cleanTime = 0;
         digitalWrite(wOnOff, LOW);
       }
    }
    if(cleanStep == 4){
      digitalWrite(pump2, HIGH);
      s_state[2] = true;
      if(s_val[8] == true){
        digitalWrite(pump2, LOW);
        s_state[2] = false;
        cleanStep = 5;
      }  
    }  
    if(cleanStep == 5){
      if(s_val[7] == true){
       digitalWrite(pump3, HIGH);
       s_state[3] = true;
       cleanTime = 0;
      }      
      if(s_val[7] == false){          
        if(cleanTime > 180){
          digitalWrite(pump3, LOW);  
          s_state[3] = false; 
          cleanTime = 0;
          nowWater = false;
          cleanStep = 1; 
          String cid = "gLinse-" + String(fCycleMode);  
          emitData(cid, fCycleMode); 
          initPump();
          delay(100);
          initialData();
          if(filterMode == 2 || filterMode == 3){
            fCycleMode = 2;
          }else{
            fCycleMode = 2;
          } 
        }
      }
      if(s_val[0] == true){
        digitalWrite(pump3, LOW);  
        s_state[3] = false; 
      }
    } 
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
   emitCnt = 0;
   Serial.println("**************************"); 
   String Str = (String)payload;  
   Serial.println(Str);
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

     Serial.print("p1: ");
     Serial.println(p1);
     Serial.print("p2: ");
     Serial.println(p2);
     Serial.print("p3: ");
     Serial.println(p3);
     Serial.print("p4: ");
     Serial.println(p4);
     Serial.print("p5: ");
     Serial.println(p5);

     p1V = p1.toInt();
     p2V = p2.toInt();
     p3V = p3.toInt();
     p4V = p4.toInt();
     p5V = p5.toInt(); 
          
     phoneId = phoneid;
     fPower = fpower; 
     if(fcyclemode == 3 || fcyclemode == 4){
       if(s_val[7] == 0){
        fCycleMode = fcyclemode;
       } 
     }else{
      fCycleMode = fcyclemode;
     }  
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
     Serial.println(phoneId);
     Serial.println(tMode);
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
    int sa = analogRead(SenA);
    int sb = analogRead(SenB);
    int sc = analogRead(SenC); 
 
    Serial.println("sensing data");
    Serial.println(sa);
    Serial.println(sb);
    Serial.println(sc);    
    //물에 잠기지않았을때 true, 잠기면 false 로 설정됨... 

    if(sa<=1700){
      s_val[2] = true;
      s_val[7] = true;
      s_val[8] = true;
    }
    if(sa>1700 && sa<=2500){
      s_val[2] = true;
      s_val[7] = true;
      s_val[8] = false;
    }
    if(sa>2500 && sa<=3300){
      s_val[2] = true;
      s_val[7] = false;
      s_val[8] = false;
    }
    if(sa>3300){
      s_val[2] = false;
      s_val[7] = false;
      s_val[8] = false;
    } 
        
    if(sb<=1700){
      s_val[1] = true;
      s_val[4] = true;
      s_val[0] = true;
    }
    if(sb>1700 && sb<=2500){
      s_val[1] = true;
      s_val[4] = true;
      s_val[0] = false;
    }
    if(sb>2500 && sb<=3300){
      s_val[1] = true;
      s_val[4] = false;
      s_val[0] = false;
    }
    if(sb>3300){
      s_val[1] = false;
      s_val[4] = false;
      s_val[0] = false;
    } 
    
    if(sc<=1700){
      s_val[5] = true;
      s_val[6] = true;
      s_val[9] = true;
    }
    if(sc>1700 && sc<=2500){
      s_val[5] = true;
      s_val[6] = true;
      s_val[9] = false;
    }
    if(sc>2500 && sc<=3300){
      s_val[5] = true;
      s_val[6] = false;
      s_val[9] = false;
    }
    if(sc>3300){
      s_val[5] = false;
      s_val[6] = false;
      s_val[9] = false;
    }    
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
      s_state[7] = true;
   }else{
      digitalWrite(plug1, LOW);
      s_state[7] = false;
   }
   if(p2V > 0){
      digitalWrite(plug2, HIGH);
      s_state[8] = true;
   }else{
      digitalWrite(plug2, LOW);
      s_state[8] = false;
   }
   if(p3V > 0){
      digitalWrite(plug3, HIGH);
      s_state[9] = true;
   }else{
      digitalWrite(plug3, LOW);
      s_state[9] = false;
   }
   if(p4V > 0){
      digitalWrite(plug4, HIGH);
      s_state[10] = true;
   }else{
      digitalWrite(plug4, LOW);
      s_state[10] = false;
   }
   if(p5V > 0){
      digitalWrite(plug5, HIGH);
      s_state[11] = true;
   }else{
      digitalWrite(plug5, LOW);
      s_state[11] = false;
   }
}

void cmdAnalysis(String payload){ 
   Serial.println(payload);
   int spIndex = payload.indexOf('`');
   int lpIndex = payload.length();
   c_val = payload.substring(0, spIndex);
   if(c_val == NULL || c_val == ""){
    c_val = "none"; 
   }
   Serial.println("___________");  
   Serial.println(c_val);
   String Str = payload.substring(spIndex+1, lpIndex);
   Serial.println(Str);
   Serial.println("___________");  
   char * cmd  = new char[Str.length() + 1];
     for(int i = 0; i < Str.length(); i++){
       cmd[i] = Str[i];
     }   
     deserializeJson(temperDocs, cmd);
     JsonObject tempObj = temperDocs.as<JsonObject>();
     Serial.println("##########"); 
     float cu = tempObj["CU"]; 
     float te = tempObj["TE"];
     String id = tempObj["ID"];
     int st = tempObj["ST"];
     temperStamp = st;
     temper_current =  floor(cu*10)/10;
     temperature =  floor(te*10)/10;
     Serial.println("temp--------");
     Serial.println(temperature);
   delete cmd;        
} 

void initPump(){
   digitalWrite(wOnOff, LOW); 
   digitalWrite(wDirection, LOW); 
   digitalWrite(pump1, LOW); 
   digitalWrite(pump2, LOW); 
   digitalWrite(pump3, LOW);
   digitalWrite(pump4, LOW);
   digitalWrite(pump5, LOW);
   s_state[0] = false;
   s_state[13] = false;
   s_state[1] = false;
   s_state[2] = false;
   s_state[3] = false;
   s_state[4] = false;
   s_state[5] = false;

}

void controlFilter(){
  controlSolenoid();
  supplyWater();
  //filterMode;

//  if(nowWater==true && fCycleMode == 2){
//    fCycleMode = 4;
//    cleanStep =4;
//  }
  //if(nowWater == false){

    if(fCycleMode != fCycleModeTemp){
        fCycleModeTemp = fCycleMode;  
        initPump();
    }

    if(fCycleMode == 1){ 
        cycleInitial = true;
        Serial.println("1번단계");
    }
    if(fCycleMode == 2){ 
        cycleWater();
        Serial.println("2번단계");
    }
    if(fCycleMode == 3){
        cycleInitial = true;
        cleanWater();
        Serial.println("3번단계");
    }
    if(fCycleMode == 4){
        cycleInitial = true;
        linseWater();
        Serial.println("4번단계");
    }   
}

void emitData(String cid, int modes){ 
   String pState = String(s_state[1])+String(s_state[2])+String(s_state[3])+String(s_state[4])+String(s_state[5])+String(s_state[6])+String(s_state[7])+String(s_state[8])+String(s_state[9])+String(s_state[10])+String(s_state[11])+String(s_state[12])+String(s_state[0])+String(s_state[13]);
   String sState = String(s_val[0])+String(s_val[1])+String(s_val[2])+String(s_val[3])+String(s_val[4])+String(s_val[5])+String(s_val[6])+String(s_val[7])+String(s_val[8])+String(s_val[9]);
   String myData = "{'temperature':"+String(temperature)+ ",'tw':" + String(temperWorking) + ",'cv':'" + String(c_val) + "', 'tCurrent':"+String(temper_current)+ ", 'cycleSeq':"+String(fCycleMode)+ ", 'sState':'"+ sState + "', 'pState':'"+ pState + "', 'current':"+String(current_val)+ ", 'tc':"+String(99)+ ", 'tc':"+String(99)+ "}";
   String muxCmd = "{'id':'" + espID + "','pid':'" + phoneId + "','cid':'" + cid + "','data':"+ myData + ",'ts':'" + timeStamp + "'}" ;
   sData =    '\"' + muxCmd + '\"';   
   const char* cstr = sData.c_str();
   if(webSocket.connect_state == "discon"){
      webSocket.disconnect(); 
      webSocket.emit("new user", Identify);
   }
   if(webSocket.connect_state == "con"){ 
    if(cmdTemp != muxCmd){
      cmdTemp = muxCmd;
      emitCnt = 0;
      Serial.println("Emit data***********");
      webSocket.emit("esp message",cstr ); 
    }     
   }  
} 

void checkAccident(){
 if(s_val[2] == false){
   pinInit();
 }
}

int stampCnt = 0;
void loop() {  

    ///----check the serial port----
      bool cmdFlag = false;
      String cmdTemp = ""; 
      while(Serial2.available()){    
        cmdTemp = Serial2.readString(); 
        cmdFlag = true;
      }
      if(cmdFlag){
        Serial.println("통보문받기끝...");
        Serial.println(cmdTemp);   
        cmdFlag = false; 
        cmdAnalysis(cmdTemp);           
      }       
      wm.process(); 
      webSocket.loop();
                  
    if((WiFi.status() == WL_CONNECTED)){   
         
      uint32_t now = millis();  
      if(abs(now - messageTimestamp) > 1000){   
        Serial.println("working..."); 
         messageTimestamp = now;    
         stampCnt++;
         muxSnData(); 
          
         if(emitCnt > 5){ 
           Serial.println("재기동....");
           Serial.println(emitCnt);
           Serial.println("************");
           ESP.restart(); 
         }
         if(webSocket.connect_state =="discon"){
            Serial.println("*********disconnected*******");
            webSocket.disconnect(); 
            emitCnt++;
            webSocket.emit("new user",Identify );
         }
         if(s_val[2] == true){
            minCounter();
            controlPlug();
            controlFilter(); 
         }else{
            pinInit();
         } 
         emitData("gGen", fCycleMode);  
         temperCnt++;
         if(temperCnt>4){
          temperCnt = 0;
          if(temperStamp == temperStampTemp){
            temperWorking = false;
          }else{
            temperStampTemp = temperStamp;  
            temperWorking = true;
          }
         }  
      }   
    }else{ 
 
    } 
}
