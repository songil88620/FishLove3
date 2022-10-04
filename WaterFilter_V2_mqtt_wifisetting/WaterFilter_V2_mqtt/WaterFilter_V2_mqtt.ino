#include <BluetoothSerial.h>
#include <EEPROM.h>
#include <Arduino.h>  
#include <WiFiClient.h> 
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>   
#include <HTTPClient.h> 
#include <math.h> 
#include <ArduinoJson.h>  
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"   
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
const size_t settingData = JSON_OBJECT_SIZE(2048);
StaticJsonDocument<settingData> settingDocs;
const size_t temperData = JSON_OBJECT_SIZE(256);
StaticJsonDocument<temperData> temperDocs;
DynamicJsonDocument meshDocs(1024);  

BluetoothSerial SerialBT; 
WiFiManager wm;
WiFiClient espClient;
PubSubClient client(espClient);
//SocketIoClient webSocket; 
const char* mqtt_server = "192.168.1.39";

String espID = ""; 
String ID = "";
const char* Identify = "";  
uint32_t messageTimestamp = 0;  
int oneMin = 0;
bool wasAccident = false;


String cmd = "";
String cmdTemp = "";
String sData = "";
String msData = "";  
int emitCnt = 0;
int temperStamp = 0;
int temperStampTemp = 0;
int temperCnt = 0;
bool temperWorking = false;
//void controlCommand(const char* payload, size_t length);
//void messageEventHandler(const char* payload, size_t length);
//--working commands variable--
 

//--sensor pin layout--
const int  SenA = 39;
const int  SenB = 34;
const int  SenC = 35;  
//--wash motor layout--
const int  wOnOff = 14;
const int  wDirection = 12; 
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
String temper_current = "0,0,0,0";
float temperature = 0;
String c_val = ""; 

//--- 
int lastStage = 1;

//---processing stage---
bool fPower = true;          // power of filter
int fCycleMode = 0;          //stage: water supply 1, cycle 2, clean 3, lense 4
int fCycleModeTemp = 0;
int filterMode = 0;          //work mode : manual 1, auto 2, AI 3 
bool sPump = true;           //supply cycle pump
bool autoPlayNow = false;
int cleanStep = 1;           //in clean stage...
int cleanTime = 0;
int serverTimecmd = 0;       //this time is the server time ex: 17*60+56  17:56
uint32_t timeStamp = 0;      //this is timestamp converted second ex:1629477685
uint32_t forceS = 0;         //force starting time with manual button press 
bool startFlag = false;
//---heater setting value---
float eTemper = 0;
float sTemper = 0;
float tSpeed = 1;
bool tMode = false;
bool tPower = false;

//플러그의 전원유지상태 남은 시간 ...분
int p1V = 0;
int p2V = 0;
int p3V = 0;
int p4V = 0;
int p5V = 0;

String pTopic = "d/p";
String sTopic = "";
String cTopic = "";
String dTopic = "d/autocheck";
String aTopic = "d/p/a";

#define RXD2 16
#define TXD2 17

const char* ssid = "Default SSID";
const char* passphrase = "Default passord";
String myWifi = "";


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
    myWifi = WiFi.macAddress();
    pinSet();
    espID = "sii00" + WiFi.macAddress();
    sTopic = "s/d/" + espID;
    cTopic = "s/autocheck/" + espID;
    ID = '\"' + espID + '\"';
    Identify = ID.c_str();     
    Serial.begin(115200);   
    Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);
    SerialBT.begin("FishLove");
    WiFi.disconnect();
    
    EEPROM.begin(512); 
    String esid = "";
    String epass = "";
    for (int i = 0; i < 32; ++i)
    {
        esid += char(EEPROM.read(i));
    }
    // esid.replace(" ", ""); 
    for (int i = 32; i < 96; ++i)
    {
        epass += char(EEPROM.read(i));
    } 
    epass.replace("\n","");     
    //WiFi.begin(esid.c_str(), epass.c_str());
    Serial.println(esid);
    Serial.println(epass);
    WiFi.begin(esid.c_str(), epass.c_str());
    if(checkWifi()){ 
        Serial.println("connected...yeey :)");        
        client.setServer(mqtt_server, 1883);
        client.setCallback(callback);  
    }  
  
//    WiFi.mode(WIFI_STA);    
//    if(wm.autoConnect("鱼的爱")){ 
//          Serial.println("connected...yeey :)");        
//          client.setServer(mqtt_server, 1888);
//          client.setCallback(callback);  
//    } 

    muxSnData();
    initialData(); 
    Serial.println("setup end...");
    delay(1000);
}

bool checkWifi(void) {
    int c= 0;
    Serial.println("Waiting for Wifi to connect");
    while(c < 20 ){
        if(WiFi.status() == WL_CONNECTED){
        return true;
        }
        delay(500);
        Serial.print("**");
        c++;
    }
    Serial.println("Connect time out, check the setting");
    return false;
}

const char* apiServer = "http://192.168.1.39:8000/init_data";
void initialData(){ 
    String sensorReadings = httpGETRequest(apiServer);
    Serial.println("API data");
    Serial.println(sensorReadings);
    const char* payload = sensorReadings.c_str(); 
    controlCommand(payload, 0); 
}

void callback(char* topic, byte* message, double length) {
    Serial.print("Message arrived on topic: ");
    Serial.println(topic);
    Serial.print(String(length));
    Serial.print(". Message: ");
    String messageTemp; 
    for (int i = 0; i < length; i++) {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println(); 
    if (String(topic) == sTopic) {
        const char* payload = messageTemp.c_str();  
        controlCommand(payload, 0);
    }
    
    // 자동모드동작명령  
    if (String(topic) == cTopic){
        const char* payload = messageTemp.c_str();
        Serial.println(">>>");
        Serial.println(payload);
        autoCommand(payload, 0);
        // action();
    }
}

void reconnect() { 
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection..."); 
    if (client.connect(espID.c_str())) {
         Serial.println("connected");  
    } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds"); 
        delay(5000);
    }
  }
  client.subscribe(sTopic.c_str());
  client.subscribe(cTopic.c_str());
}

String httpGETRequest(const char* serverName) {
  WiFiClient clients;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(clients, serverName); 
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
    if(s_state[6] == false){
        delay(1000);
        digitalWrite(valve1, HIGH);
        delay(1000);
    } 
    s_state[6] = true;
    Serial.println("쏠레노이드 열기");
  }
  if(s_val[5] == false){
    if(s_state[6] == true){
        delay(1000);
        digitalWrite(valve1, LOW);
        delay(1000);
    }  
    s_state[6] = false;
    Serial.println("쏠레노이드 닫기");
  }  
}  
void supplyWater(){
  if(sPump == true){
    if(s_val[9] == true){
        if(s_state[5] == true){
            delay(1000);
            digitalWrite(pump5, LOW); 
        }  
        s_state[5] = false;
        Serial.println("펌프5 닫기");
    //alert alarm
    }else{
        if(s_state[5] == false){
            delay(1000);
            digitalWrite(pump5, HIGH);
            delay(1000);
        }  
        s_state[5] = true;
        Serial.println("펌프5 열기");
    }
  }else{
    delay(1000);
    digitalWrite(pump5, LOW); 
    s_state[5] = false;
  } 
}

bool cycleInitial = true;

void cycleWater(){
    if(cycleInitial){
    cycleInitial = false;
    if(s_state[4] == false && s_val[1] == true){
        delay(1000);
        digitalWrite(pump4, HIGH); 
    } 
    Serial.println("펌프4 열기");
    s_state[4] = true;
    }
    if(s_val[1] == false){
        Serial.println("펌프1 열기??");
    if(s_state[4] == true){ 
        digitalWrite(pump4, LOW); 
    } 
    if(s_state[1] == false){
        delay(1000);
        digitalWrite(pump1, HIGH); 
    } 
    Serial.println("여기****"); 
    s_state[4] = false;
    s_state[1] = true;     
    Serial.println("펌프4 닫기??");
    }
    if(s_val[4] == true){
        if(s_state[1] == true){
        delay(1000);
        digitalWrite(pump1, LOW);
        delay(1000);
        } 
        if(s_state[4] == false){
        delay(1000);
        digitalWrite(pump4, HIGH); 
        delay(1000);
        } 
        s_state[4] = true;
        s_state[1] = false;     
        Serial.println("펌프4 열기");  
    } 
    lastStage = 2;  
}

bool nowWater = false;

void cleanWater(){
       
    if(s_val[4] == true){
        if(s_state[4] == false){
            delay(1000);
            digitalWrite(pump4, HIGH);  
            delay(1000);
        }  
        s_state[4] = true;
        Serial.println("펌프4 열기");
    }
    if(s_val[1] == false){  
        if(s_state[4] == true){
            delay(1000);
            digitalWrite(pump4, LOW); 
            delay(1000);
        }     
        s_state[4] = false;   
        Serial.println("펌프4 닫기"); 
    }  
    nowWater = true;
    cleanTime++;
    Serial.print("clenaTime:");
    Serial.println(cleanTime);
    Serial.println(cleanStep);
    if(cleanStep == 1){
      if(s_state[1] == true){
       delay(1000);
       digitalWrite(pump1, LOW);
       delay(1000); 
      } 
      s_state[1] = false;   
    }
    if(cleanTime > 60 && cleanStep == 1){ // 정해진 시간동안 준비 3분 동안 대기
      cleanStep = 2;   
      cleanTime = 0;
    }
    if(cleanStep == 2){
      Serial.println("2번펌프열기");
      if(s_state[2] == false){
        delay(1000);
        digitalWrite(pump2, HIGH);
        delay(1000);
      } 
      s_state[2] = true;   
      if(s_val[7] == true){
        if(s_state[2] == true){
            delay(1000);
            digitalWrite(pump2, LOW);
            delay(1000);
        }
        
        s_state[2] = false;
        Serial.println("2번펌프닫기");
        cleanStep = 3;
        cleanTime = 0;
      }
    }
    if(cleanStep == 3){   // 15분동안 3번단계에서 작업 정회전,역회전,정지
       int remain = cleanTime % 20;
       if(remain>=1 && remain <=8 && cleanTime <= 60){ 
         if(s_state[13] == false){
            delay(1000);
            digitalWrite(wDirection, HIGH);
            delay(1000);
         } 
         if(s_state[0] == false){
            delay(1000);
            digitalWrite(wOnOff, HIGH);
            delay(1000);
         }  
         s_state[13] = true;
         s_state[0] = true; 
         Serial.println("+ 회전");
       }else if(remain>=11 && remain<=18 && cleanTime <= 60){
         if(s_state[13] == true){
            delay(1000);
            digitalWrite(wDirection, LOW);
            delay(1000);
         } 
         if(s_state[0] == false){
            delay(1000);
            digitalWrite(wOnOff, HIGH);
            delay(1000);
         } 
         s_state[13] = false;
         s_state[0] = true; 
         Serial.println("- 회전");
       }else{ 
         if(s_state[0] == true){
            delay(1000);
            digitalWrite(wOnOff, LOW); 
            delay(1000);
         } 
         if(s_state[13] == true){
            delay(1000);
            digitalWrite(wDirection, LOW);
            delay(1000);
         } 
         s_state[0] = false;
         s_state[13] = false;
         Serial.println("0 회전");
       }
       //if(cleanTime > 900){
       if(cleanTime > 63){
         cleanStep = 4;
         cleanTime = 0;
         if(s_state[0] == true){
            delay(1000);
            digitalWrite(wOnOff, LOW); 
            delay(1000);
         }  
         delay(2000);
         s_state[0] = false; 
       }
    }
    if(cleanStep == 4){
      lastStage = 3;  
      Serial.println("소4번단계...");
      if(s_state[2] == false){
        delay(1000);
        digitalWrite(pump2, HIGH);
        delay(1000);
      } 
      s_state[2] = true; 
      Serial.println("2번펌프열기");
      if(s_val[8] == true){
        if(s_state[2] == true){
            delay(1000);
            digitalWrite(pump2, LOW);
            delay(1000);
        } 
        s_state[2] = false; 
        Serial.println("2번펌프닫기");
        cleanStep = 5;
      }  
    }  
    if(cleanStep == 5){
      if(s_val[7] == true){
        if(s_state[3] == false){
            delay(1000);
            digitalWrite(pump3, HIGH);
            delay(1000);
        } 
       s_state[3] = true; 
       Serial.println("3번펌프열기");
      }      
      if(s_val[7] == false){
        if(s_state[3] == true){
            delay(1000);
            digitalWrite(pump3, LOW); 
            delay(1000);
        } 
        s_state[3] = false; 
        Serial.println("3번펌프닫기");
        //clean end... 
        cleanTime = 0; 
        nowWater = false;
        cleanStep = 1;
        String cid = "gLinse-" + String(fCycleMode);  
        emitData(cid, fCycleMode); 
        initPump();
        delay(1000);
        lastStage = 3;
        //initialData();
        if(filterMode == 2 || filterMode == 3 || (wasAccident == true && (filterMode == 2 || filterMode == 3))){
            fCycleMode = 4;  
            fCycleModeTemp = fCycleMode;
        }else{
            if(fCycleModeTemp == fCycleMode){
                fCycleMode = 1;
            }          
            fCycleModeTemp = fCycleMode;
        }
        
      }  
      if(s_val[0] == true){
        if(s_state[3] == true){
            delay(1000);
            digitalWrite(pump3, LOW);
            delay(1000);  
        } 
        s_state[3] = false;  
      } 
    } 
}

void linseWater() { 
    if(s_val[4] == true){
        if(s_state[4] == false){
            delay(1000);
            digitalWrite(pump4, HIGH);  
            delay(1000);
        }  
        s_state[4] = true;
        Serial.println("펌프4 열기");
    }
    if(s_val[1] == false){  
        if(s_state[4] == true){
            delay(1000);
            digitalWrite(pump4, LOW); 
            delay(1000);
        }     
        s_state[4] = false;   
        Serial.println("펌프4 닫기"); 
    }  
    cleanTime++;
    nowWater = true;
    Serial.print("clenaTime:  LINSE........");
    Serial.println(cleanTime);

    if(lastStage == 3){ 
        cleanStep = 3;
        cleanTime = 0;
    }
     
    if(cleanStep == 1){
        if(s_state[1] == true){
            delay(1000);
            digitalWrite(pump1, LOW);
            delay(1000); 
        } 
        s_state[1] = false;   
    }
    
    if(cleanTime > 60 && cleanStep == 1){  //// 정해진 시간동안 준비 10
        cleanStep = 2;
        cleanTime = 0;
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
        lastStage = 4;
        int remain = cleanTime % 20;
        if(remain>=1 && remain <=8 && cleanTime <= 60){  
            if(s_state[13] == false){
            delay(1000);
            digitalWrite(wDirection, HIGH);
            delay(1000);
            } 
            if(s_state[0] == false){
            delay(1000);
            digitalWrite(wOnOff, HIGH);
            delay(1000);
            }  
            s_state[13] = true;
            s_state[0] = true; 
            Serial.println("+ 회전");
        }else if(remain>=11 && remain<=18 && cleanTime <= 60){
            if(s_state[13] == true){
            delay(1000);
            digitalWrite(wDirection, LOW);
            delay(1000);
            } 
            if(s_state[0] == false){
            delay(1000);
            digitalWrite(wOnOff, HIGH);
            delay(1000);
            } 
            s_state[13] = false;
            s_state[0] = true; 
            Serial.println("- 회전"); 
        }else{ 
            if(s_state[0] == true){
            delay(1000);
            digitalWrite(wOnOff, LOW); 
            delay(1000);
            } 
            if(s_state[13] == true){
            delay(1000);
            digitalWrite(wDirection, LOW);
            delay(1000);
            } 
            s_state[0] = false;
            s_state[13] = false;
            Serial.println("0 회전"); 
        }
        //if(cleanTime > 900){ 
        if(cleanTime > 63){
            cleanStep = 4;
            cleanTime = 0;
            if(s_state[0] == true){
            delay(1000);
            digitalWrite(wOnOff, LOW); 
            delay(1000);
            }  
            delay(2000);
            s_state[0] = false; 
        }
    }
    if(cleanStep == 4){ 
        if(s_state[2] == false){
            delay(1000);
            digitalWrite(pump2, HIGH);
            delay(1000);
        } 
        s_state[2] = true;
        if(s_val[8] == true){
            if(s_state[2] == true){
            delay(1000);
            digitalWrite(pump2, LOW);
            delay(1000);
            }        
            s_state[2] = false;
            cleanStep = 5;
        }  
    }  
    if(cleanStep == 5){
        if(s_val[7] == true){
            if(s_state[3] == false){
            delay(1000);
            digitalWrite(pump3, HIGH);
            delay(1000);
            }      
            s_state[3] = true;
            cleanTime = 0;
        }      
        if(s_val[7] == false){          
            if(cleanTime > 90){  // /////// time setting
            if(s_state[3] == true){
                delay(1000);
                digitalWrite(pump3, LOW);  
                delay(1000);
            }          
            s_state[3] = false; 
            cleanTime = 0;
            nowWater = false;
            cleanStep = 1; 
            String cid = "gLinse-" + String(fCycleMode);  
            emitData(cid, fCycleMode); 
            initPump();
            delay(100);
            //initialData();
            if(filterMode == 2 || filterMode == 3){
                fCycleMode = 2; 
            }else{
                if(fCycleModeTemp == fCycleMode){
                fCycleMode = 1;
                }          
                fCycleModeTemp = fCycleMode;
            } 
            wasAccident = false; 
            lastStage = 4;
            msgWarn();
            }
        }
        if(s_val[0] == true){
            if(s_state[3] == true){
            delay(1000);
            digitalWrite(pump3, LOW);  
            delay(1000);
            }  
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

void autoCommand(const char* payload, size_t length){   
    String Str = (String)payload;  
    Serial.println(Str);
    char * cmd  = new char[Str.length() + 1];
    for(int i = 0; i < Str.length(); i++){
    cmd[i] = Str[i];
    }   
    deserializeJson(settingDocs, cmd);
    JsonObject settingObj = settingDocs.as<JsonObject>();
    Serial.println("##########"); 
    bool autoPlay = settingObj["playNow"]; 
    autoPlayNow = autoPlay;
    if(autoPlayNow == true){
    fCycleMode = 3;
    }   
    delete cmd;  
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
    bool fpower = settingObj["fp"];
    int fcyclemode = settingObj["fc"];
    int filtermode = settingObj["fm"];
    float etemper = settingObj["et"];
    float stemper = settingObj["st"];
    float tspeed = settingObj["ts"];
    bool tmode = settingObj["tm"];
    bool tpower = settingObj["tp"];
    bool spump = settingObj["sp"];

    String obj = settingObj["pg"]; 
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
        

    fPower = fpower; 
    if(fcyclemode == 3 || fcyclemode == 4){
    // if(s_val[7] == 0){
    fCycleMode = fcyclemode;
    // } 
    }else{
    fCycleMode = fcyclemode;
    }  
    filterMode = filtermode;
    eTemper = etemper;
    sTemper = stemper;
    tSpeed = tspeed;
    tMode = tmode;
    tPower = tpower;
    sPump = spump;
    Serial.println("commands......");   
    Serial.println(fCycleMode);
    delete cmd;  
} 

//처음 5분동안 시험데이터측정하고 이데이터에 기초하여 속도조절
int temperCnts = 0; 
int temperMinute = 0;
int temperSecond = 0;
float sTemperature = 0;
float eTemperature = 0;
int rs = 100;
void controlHeater() { 
  if(tPower == true){
    temperSecond++;
    temperMinute++;
    if(temperSecond > 60){
      temperSecond = 0; 
    }
    if(temperMinute > 100){
      temperMinute = 0; 
    }
    if(temperCnts == 0 && temperature > 0){
      sTemperature = temperature;
    }
    if(temperCnts == 120){
      eTemperature = temperature;
      rs = floor( tSpeed / ((eTemperature - sTemperature)* 30) * 100);
      if(rs > 100){
        rs = 100;
      }
      if(rs <= 50){
        rs = 50; 
      }
    }
    Serial.println("rs value:");
    Serial.println(rs);
    if(temperCnts<121){
      temperCnts++;
    }   

    int onCycle = temperMinute % 100;
    Serial.println("onCycle value:");
    Serial.println(onCycle);

    if(onCycle < rs){
      if(eTemper < temperature){
        digitalWrite(plug5, LOW);
        delay(500);
        digitalWrite(plug6, LOW); 
        s_state[11] = false;
        s_state[12] = false;
        Serial.println("heater off:");
      }
      if(sTemper > temperature){ 
        Serial.println("heater on:");
        digitalWrite(plug5, HIGH);
        delay(500);
        digitalWrite(plug6, HIGH);  
        s_state[11] = true;
        s_state[12] = true;
      } 
    }else{
        digitalWrite(plug5, LOW);
        digitalWrite(plug6, LOW); 
        s_state[11] = false;
        s_state[12] = false;
        Serial.println("heater off:");
    }   
      
    
  }else{
    digitalWrite(plug5, LOW);
    delay(500);
    digitalWrite(plug6, LOW); 
    s_state[11] = false;
    s_state[12] = false;
  } 
}

//void messageEventHandler(const char* payload, size_t length){
//     Serial.println("init");  
//}  

//센서자료 측정 1초에 한번
int muxCnt = 0;
int muxA = 0;
int muxB = 0;
int muxC = 0; 

void muxSnData(){ 
    int sa = analogRead(SenA);
    int sb = analogRead(SenB);
    int sc = analogRead(SenC);  
     
    //물에 잠기지않았을때 true, 잠기면 false 로 설정됨... 

    if(sa<=1000){
      s_val[2] = true;
      s_val[7] = true;
      s_val[8] = true;
    }
    if(sa>1000 && sa<=1400){
      s_val[2] = true;
      s_val[7] = true;
      s_val[8] = false;
    }
    if(sa>1400 && sa<=2500){
      s_val[2] = true;
      s_val[7] = false;
      s_val[8] = false;
    }
    if(sa>2500){
      s_val[2] = false;
      s_val[7] = false;
      s_val[8] = false;
    } 
        
    if(sb<=1000){
      s_val[1] = true;
      s_val[4] = true;
      s_val[0] = true;
    }
    if(sb>1000 && sb<=1400){
      s_val[1] = true;
      s_val[4] = true;
      s_val[0] = false;
    }
    if(sb>1400 && sb<= 2500){
      s_val[1] = true;
      s_val[4] = false;
      s_val[0] = false;
    }
    if(sb>2500){
      s_val[1] = false;
      s_val[4] = false;
      s_val[0] = false;
    } 
    
    if(sc<=1000){
      s_val[5] = true;
      s_val[6] = true;
      s_val[9] = true;
    }
    if(sc>1000 && sc<=1400){
      s_val[5] = true;
      s_val[6] = true;
      s_val[9] = false;
    }
    if(sc>1400 && sc<=2500){
      s_val[5] = true;
      s_val[6] = false;
      s_val[9] = false;
    }
    if(sc>2500){
      s_val[5] = false;
      s_val[6] = false;
      s_val[9] = false;
    }    

    Serial.println("sensor data....");
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
        //digitalWrite(plug5, HIGH);
        //s_state[11] = true;
    }else{
        // digitalWrite(plug5, LOW);
        // s_state[11] = false;
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
    String tm = payload.substring(spIndex+1, lpIndex);  
    temper_current =  c_val;
    temperature =  tm.toFloat();
    
           
} 

void initPump(){
   digitalWrite(wOnOff, LOW); 
   delay(500);
   digitalWrite(wDirection, LOW); 
   delay(500);
   digitalWrite(pump1, LOW); 
   delay(500);
   digitalWrite(pump2, LOW); 
   delay(500);
   digitalWrite(pump3, LOW);
   delay(500);
   digitalWrite(pump4, LOW); 
   delay(500);
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

//  if(nowWater==true && fCycleMode == 2){
//    fCycleMode = 4;
//    cleanStep =4;
//  }

    if(fCycleMode != fCycleModeTemp && cleanStep == 1){
        fCycleModeTemp = fCycleMode;  
        initPump();
        cleanStep = 1; 
    }

    if(fCycleMode != fCycleModeTemp && cleanStep<4){
      cleanStep = 4;
      initPump();
      cleanTime = 0;
      delay(1000);
    }    

    if(fCycleModeTemp == 1 || fCycleMode == 0){ 
        cycleInitial = true;
        Serial.println("1번단계"); 
    }
    if(fCycleModeTemp == 2){ 
        cycleWater();
        Serial.println("2번단계");
    }
    if(fCycleModeTemp == 3){
        cycleInitial = true;
        cleanWater();
        Serial.println("3번단계");
    }
    if(fCycleModeTemp == 4){
        cycleInitial = true;
        linseWater();
        Serial.println("4번단계");
    }    
}

void msgWarn(){ 
    if(filterMode == 1){
        String muxCmd = "{'id':'" + espID + "','ac':'" + String(wasAccident) + "'}";
        const char* cstr = muxCmd.c_str();  
        client.publish( aTopic.c_str(), cstr);   
    }
}

void checkAutoMode(){
  // 자동모드이면 2 
    if(filterMode == 2) {
        String muxCmd = "{'id':'" + espID + "'}"; 
        const char* cstr = muxCmd.c_str();   
        client.publish( dTopic.c_str(), cstr); 
    } 
} 

void emitData(String cid, int modes){  
    String pState = String(s_state[1])+String(s_state[2])+String(s_state[3])+String(s_state[4])+String(s_state[5])+String(s_state[6])+String(s_state[7])+String(s_state[8])+String(s_state[9])+String(s_state[10])+String(s_state[11])+String(s_state[12])+String(s_state[0])+String(s_state[13]);
    String sState = String(s_val[0])+String(s_val[1])+String(s_val[2])+String(s_val[3])+String(s_val[4])+String(s_val[5])+String(s_val[6])+String(s_val[7])+String(s_val[8])+String(s_val[9]);
    String myData = "{'tp':"+String(temperature)+ ",'tw':" + String(temperWorking) + ",'cv':'" + String(temper_current) + "', 'tr':'"+ String(cleanStep) + "', 'cs':"+String(fCycleModeTemp)+ ", 'st':'"+ sState + "', 'pt':'"+ pState + "', 'cr':"+String(wasAccident)+ ", 'tc':"+String(lastStage)+ "}";
    String muxCmd = "{'id':'" + espID +  "','fp':'" + String(fPower) +  "','cid':'" + cid + "','data':"+ myData + ",'ts':'" + timeStamp + "'}" ;
    sData =  muxCmd ;   
    const char* cstr = sData.c_str(); 
    cmdTemp = muxCmd;
    emitCnt = 0;
    Serial.println("Emit data***********"); 
    client.publish( pTopic.c_str(), cstr); 
} 

void checkAccident(){
    if(s_val[2] == false && fCycleMode !=3 ){
        pinInit();
        delay(1000);
        fCycleMode = 3;
    }
}

bool stopAll = false;
int stampCnt = 0;
int wifiCnt = 0;
void loop() { 
    //wm.process();  
    uint32_t now = millis();  
    if(abs(now - messageTimestamp) > 1000){   
        Serial.println("call in...");
        if((WiFi.status() == WL_CONNECTED)){
            if (!client.connected()) {
                reconnect();
            }
            client.loop();
            Serial.println("call in... here");       
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
            
            stampCnt++;  
            wifiCnt = 0;
            Serial.println("working..."); 
            messageTimestamp = now;   
            muxSnData(); 
            controlHeater(); 
            
            if(emitCnt > 5){ 
                Serial.println("재기동....");
                Serial.println(emitCnt);
                Serial.println("************");
                ESP.restart(); 
            } 

            if(s_val[2] == false){ 
                if((fCycleMode !=3 || fCycleMode !=4) && (filterMode == 2 || filterMode == 3)){
                //pinInit();
                fCycleMode =3;
                wasAccident = true;
                } 
                if(fCycleMode !=3 && fCycleMode !=4 &&  filterMode == 1 ){
                pinInit();  
                stopAll = true;
                fCycleModeTemp = 0;
                wasAccident = true;
                fCycleMode = 0;
                msgWarn();
                }
            } 
            if(fCycleMode ==3 || fCycleMode ==4){
                stopAll = false;
            }
            minCounter();
            controlPlug(); 
            if(stopAll == false){
                if(fPower == true){
                controlFilter(); 
                }else{
                initPump();
                fCycleModeTemp = 1;
                }            
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

            if(stampCnt > 30){
                stampCnt = 0;
                checkAutoMode();
            }
            
        }else{  
            //wifiCnt++;
            //if(wifiCnt > 20){
            // ESP.restart(); 
            // Serial.println(">>>there is no wifi");
            //} 

            if (SerialBT.available()) { 
            Serial.write(SerialBT.read());
            String inData = SerialBT.readString(); 
            int spIndex = inData.indexOf('/');
            int mpIndex = inData.lastIndexOf('/');
            int lpIndex = inData.length();
            
            String h_d = inData.substring(0, spIndex);
            int idx = h_d.indexOf('`');
            String wifi_name = h_d.substring(idx+1, h_d.length());
            String wifi_pass = inData.substring(spIndex+1, mpIndex); 
            String key = inData.substring(mpIndex+1, lpIndex).substring(0,3);
            
            
            if(key == "set"){ 
                for (int i = 0; i < 96; ++i) {
                    EEPROM.write(i, 0);
                }
                for (int i = 0; i < wifi_name.length(); ++i)
                {
                    EEPROM.write(i, wifi_name[i]); 
                    Serial.println(wifi_name[i]);
                } 
                for (int i = 0; i < wifi_pass.length(); ++i)
                {
                    EEPROM.write(32 + i, wifi_pass[i]); 
                }
                EEPROM.commit(); 
                String r = "OK";
                String sdata = r.c_str();
                for(int i = 0 ; i < sdata.length(); i++ ){
                    SerialBT.write(sdata[i]);
                } 
            }
            if(key == "mac"){
                Serial.println("here 2");
                Serial.println(myWifi);
                String sdata = myWifi.c_str();  
                for(int i = 0 ; i < sdata.length(); i++ ){
                    SerialBT.write(sdata[i]);
                } 
            }
                
            } 
            
        }   
    }  
}
