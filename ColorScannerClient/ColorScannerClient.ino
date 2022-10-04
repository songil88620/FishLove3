#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h> 
#include <ArduinoJson.h> 
 
DynamicJsonDocument meshDocs(1024); 
StaticJsonDocument<250> jsonDocument;
const char* ssid = "NodeMCU";
const char* password = "12345678";  
const char* serverName = "http://192.168.1.1:80/colorSensing";

unsigned long lastTime = 0; 
unsigned long timerDelay = 1000; 

WiFiClient client;
HTTPClient http; 
    
//For Color Sensor
#include "Adafruit_TCS34725.h"
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_16X);
int reds, greens, blues, pre_cnt, test_len;
int redmix[10]   = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
int greenmix[10] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
int bluemix[10]  = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
int mixlen = 10;

//////////////////////To initSensor Start/////////////////////////////////////

int init_red_down, init_red_up, init_green_down, init_green_up, init_blue_down, init_blue_up;

const int UP_PIN = 12;     // the number of the pushbutton pin
const int DOWN_PIN = 14;   // the number of the pushbutton pin
int upLimitState = 0;
int downLimitState = 0;
byte scans = 0;

//Variables for scan action
int lens = 0; //measurement length
int acts = 0; //maasurement state 0:steady 1:scan
byte realact = 0;

const int M_PIN = 10;   //output of motor controll

//Function Decalration 
void detectUp(void);
void detectDown(void);

 

void detectUp() {
  uint16_t c, r, g, b;
  long rd = 0, bd = 0, gd = 0;
  for (int i = 0; i < 20; i++) {
    tcs.setInterrupt(false);
    tcs.getRawData(&r, &g, &b, &c);
    tcs.setInterrupt(true);
    rd += r;
    gd += g;
    bd += b;
    Serial.println(b);
    delay(100);
  }
  rd = rd / 20;
  gd = gd / 20;
  bd = bd / 20;

  String limitdata = String(rd) + "R/" + String(gd) + "G/" + String(bd) + "B/";
  Serial.println("웃한계");
  Serial.println(limitdata);

  for (int i = 0; i < limitdata.length(); i++)
  {
    EEPROM.write(101 + i, limitdata[i]);
    Serial.println(limitdata[i]);
  }
  EEPROM.commit();    //Store data to EEPROM

}

void detectDown() {
  Serial.println("----------**************-----------");
  uint16_t c, r, g, b;
  uint16_t rd = 0, bd = 0, gd = 0;
  for (int i = 0; i < 5; i++) {
    tcs.setInterrupt(false);
    tcs.getRawData(&r, &g, &b, &c);
    tcs.setInterrupt(true);
    rd += r;
    gd += g;
    bd += b;
    Serial.println(b);
    delay(50);
  }
  rd = rd / 5;
  gd = gd / 5;
  bd = bd / 5;

  // String reddata = String(rd);
  // String greendata = String(gd);
  // String bluedata = String(bd);

  String  limitdata = String(rd) + "R/" + String(gd) + "G/" + String(bd) + "B/";
  Serial.println("아래한계");
  Serial.println(limitdata);

  for (int i = 0; i < limitdata.length(); i++)
  {
    EEPROM.write(131 + i, limitdata[i]);
    Serial.println(limitdata[i]);
  }
  //    String ss = "";
  //    for (int i = 131; i < 160; ++i)
  //     {
  //      ss += String(EEPROM.read(i));
  //     }
  //    Serial.println(ss);
  EEPROM.commit();    //Store data to EEPROM

}


void setup()
{
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);
  pinMode(M_PIN, OUTPUT);  
  digitalWrite(M_PIN, LOW); 
  
  Serial.begin(115200);  
//  innercom.begin(9600);
  Serial.println(); 
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10); 
  //----------------------Sensor initValue Loading------------------------...
  String upInit = "";
  String downInit = "";
  for (int i = 101; i < 130; ++i)
  {
    upInit += char(EEPROM.read(i));
  }
  int index_1 = upInit.indexOf("R/");
  int index_2 = upInit.indexOf("G/");
  int index_3 = upInit.indexOf("B/");
  init_red_up   = (upInit.substring(0, index_1)).toInt();
  init_green_up = (upInit.substring(index_1 + 2, index_2)).toInt();
  init_blue_up  = (upInit.substring(index_2 + 2, index_3)).toInt(); 

  for (int i = 131; i < 160; ++i)
  {
    downInit += char(EEPROM.read(i));
  }
  index_1 = downInit.indexOf("R/");
  index_2 = downInit.indexOf("G/");
  index_3 = downInit.indexOf("B/");
  init_red_down   = (downInit.substring(0, index_1)).toInt();
  init_green_down = (downInit.substring(index_1 + 2, index_2)).toInt();
  init_blue_down  = (downInit.substring(index_2 + 2, index_3)).toInt(); 

   if (tcs.begin()) {
      Serial.println(F("Found Sensor"));
    }
    else {
      Serial.print(F("No Found Sensor"));
    }
    return;  
  
}

void callServer(){
    if(WiFi.status()== WL_CONNECTED){  
      http.begin(client, serverName);  
      http.addHeader("Content-Type", "text/plain");  
      meshDocs["ID"] = "temperature";                      
      // meshDocs["CU"] = nCurrent;   
      meshDocs["ST"] = millis();
      // sensors.requestTemperatures();          
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
          
      Serial.println(httpResponseCode);  
      http.end();
    }

}

void gCmdTimer(){  
 cmdInterface(); 
}

void color_read() {
  uint16_t clea, red, green, blue;
  tcs.setInterrupt(false); //
  tcs.getRawData(&red, &green, &blue, &clea); //
  tcs.setInterrupt(true); // turn off LED

  Serial.print(" RED :");
  Serial.print(red);
  Serial.print(" GREEN :");
  Serial.print(green);
  Serial.print(" BLUE :");
  Serial.print(blue);

  reds = map(red, init_red_down, init_red_up, 0, 255);
  greens = map(green, init_green_down, init_green_up, 0, 255);
  blues = map(blue, init_blue_down, init_blue_up, 0, 255);

      if(reds>255){
          reds = 255;  
      }
      if(reds<0){
          reds = 0;
      }
      if(greens>255){
          greens = 255;  
      }
      if(greens<0){
          greens = 0;
      }
      if(blues>255){
          blues = 255;  
      }
      if(blues<0){
          blues = 0;
      }
  Serial.println("---------------------------------------------"); 
  Serial.print(" RED :");
  Serial.print(reds);
  Serial.print("     GREEN :");
  Serial.print(greens);
  Serial.print("      BLUE :");
  Serial.print(blues);
  Serial.println("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");

}


byte measure_color[] = {0};  
String scan_result = ""; 

void color_scan(){
  realact = 1;
  uint16_t clea, red, green, blue;
  int mred=0, mgreen=0, mblue=0;
  pre_cnt = 0;
  byte cindex = 0;
  String color_send = "";

  while(lens>0){
    
   tcs.setInterrupt(false);  
   tcs.getRawData(&red, &green, &blue, &clea);  
   tcs.setInterrupt(true);  
   reds = map(red, init_red_down, init_red_up*0.98, 0, 255);
   greens = map(green, init_green_down, init_green_up*0.98, 0, 255);
   blues = map(blue, init_blue_down, init_blue_up*0.98, 0, 255);
   delay(50);

      if(reds>255){
          reds = 255;  
      }
      if(reds<0){
          reds = 0;
      }
      if(greens>255){
          greens = 255;  
      }
      if(greens<0){
          greens = 0;
      }
      if(blues>255){
          blues = 255;  
      }
      if(blues<0){
          blues = 0;
      }

      Serial.println("^^^^^**********^^^^");
      Serial.println(reds);
      Serial.println(greens);
      Serial.println(blues);
      Serial.println("^^^^^**********^^^^");

      if(reds == 255&&greens == 255 && blues == 255){
        pre_cnt = 0;
        mred = 0;
        mgreen = 0;
        mblue = 0;  
      }
      else{
        if(reds > 10 || greens > 10 || blues > 10){
          pre_cnt++;

          if(pre_cnt > 31){    /////if(pre_cnt > 5){   30~45번사이 자료채집
              if(pre_cnt <42){   ///if(pre_cnt <15){
                mred += reds;
                mgreen += greens;
                mblue += blues;  
              } 
              if(pre_cnt == 41){
                mred  = mred / 10;
                mgreen = mgreen / 10;
                mblue = mblue / 10; 
                measure_color[cindex] = mred;
                measure_color[cindex + 1] = mgreen;
                measure_color[cindex + 2] = mblue;
                color_send = color_send + mred + "," + mgreen + "," + mblue + "-";
                Serial.println("++++++++++++++++++++");
                Serial.println(color_send);
                Serial.println("++++++++++++++++++++");
                cindex++;  //한개의 색상에 대해 측정끝나면 +
                lens-- ;
                Serial.println("완성");
                Serial.println(lens);
                Serial.println(pre_cnt); 
              } 
          }  
        }
        else{
          Serial.println("**************"); 
        }  
      } 
     
  }//while
  
  realact = 0; 
  scan_result = color_send.substring(0, color_send.length()-1);
       
  Serial.println(scan_result);
  Serial.println("측정끝...............");   
 
  cmdInterface();
  delay(15000);
  digitalWrite(M_PIN, LOW);
  delay(150000); 
  ESP.restart();
  acts = 0;
}

void cmdInterface(){   
        DynamicJsonDocument doc(1024);   
        //doc["cl"] = "250, 12, 57-140, 154, 227-118, 20, 23-208, 137, 185";  
        //Serial.println(scan_result);
        Serial.println("측정결과반환...");
        doc["cl"] = scan_result; 
        String msg;
        serializeJson(doc, msg);
        Serial.println("&&&&&&&&&");
        Serial.println(acts);

   
    Serial.println("여기???");
//    for(int i = 0; i < msg.length(); i ++){
//       innercom.write(msg[i]);
//    } 
    Serial.println("통보문보내기........");
    Serial.println(msg);
 
      
} 

void cmdAnalysis(String cmdMsg){ 
      //{"st":0,"sl":4} 
      DynamicJsonDocument doc(1024); 
      DeserializationError error = deserializeJson(doc, cmdMsg);  
      String actss = doc["st"];
      String lenss = doc["sl"];
      acts = actss.toInt();
      lens = lenss.toInt();  

      if(acts == 1){
        digitalWrite(M_PIN, HIGH);
        scans = 1;
        color_scan(); 
        scans = 0;
      }
      else{
        digitalWrite(M_PIN, LOW);
      }
         
}   

void loop() {
  upLimitState = 0;
  downLimitState = 0;
  upLimitState = digitalRead(UP_PIN);
  downLimitState = digitalRead(DOWN_PIN); 
 
  if (upLimitState == HIGH) {
    scans = 1;
    Serial.println("H");
    detectUp();
    delay(300);
    scans = 0;
  }
  if (downLimitState == HIGH) {
    scans = 1;
    detectDown();
    delay(300);
    Serial.println("L");
    scans = 0;
  } 
    if(acts == 0 && scans == 0){
      String cmdTemp = ""; 
      bool cmdFlag = false;
//      while(innercom.available()){   //centerO로 부터 명령대가중...
//        cmdTemp = innercom.readString(); 
//        cmdFlag = true;
//      }
      if(cmdFlag){
        Serial.println("통보문받기끝...");
        Serial.println(cmdTemp);   
        cmdAnalysis(cmdTemp);        //명령해석 및 처리...      
        gCmdTimer();
        cmdFlag = false; 
        
      } 
    }
     
   
}
