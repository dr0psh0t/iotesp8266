#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Wire.h>

#include <RtcUtility.h>
#include <RtcDS3231.h>
#include <RtcDateTime.h>
#include <TimeLib.h>
#include <Time.h>

#include <NTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266HTTPClient.h>
#include "ArduinoJson.h"
#include <FS.h>

//  enter host.local in browser
const char* host = "stefor100035";
const char* hostName = "stefor100035";

int sec_elapsed = 0;
int noWorkPassed = 0;
int success;
int statusCode;
int inwork = 0;
char HH = 00;
char MM = 00;
char hours, minutes, seconds;
long dTime;
long startTime;
long startTime2;
long val0;
long val1;
long val2;
long val3;
const int GMT_8 = 28800;
String response;
String configjson;
String configcontent;
String errorcontent;
String authFormContent;
bool isOnline = false;

int networkid1conf = 0;
int networkid2conf = 0;
int subnetconf = 0;
int hostconf = 0;
const char* ntpipconf = "";
String mcdapiendpointconf = "";
int midconf = 0;
String wificonf = "";
String passwordconf = "";
int gatewaydnshostconf = 0;
String usernameconf = "";
String userpassconf = "";

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
HTTPClient http;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "192.168.1.100", GMT_8);
RtcDS3231<TwoWire> Rtc(Wire);
#define countof(a) (sizeof(a) / sizeof(a[0]))
LiquidCrystal_I2C lcd(0x27,16,2);

void setup() {
  Serial.begin(115200);
  pinMode(D5, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D6, OUTPUT);
  
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  
  if (!SPIFFS.begin()) {
    lcd.setCursor(0, 0);
    lcd.print("SPIFFS Mount");
    lcd.setCursor(0, 1);
    lcd.print("Failed");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("SPIFFS Mount");
    lcd.setCursor(0, 1);
    lcd.print("Successful");
    readConfig();
  }
  
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.onStart([]() {});
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
  ArduinoOTA.onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR) {}
    else if (error == OTA_BEGIN_ERROR) {}
    else if (error == OTA_CONNECT_ERROR) {}
    else if (error == OTA_RECEIVE_ERROR) {}
    else if (error == OTA_END_ERROR) {}
  });
  ArduinoOTA.begin();

  timeClient.begin();
  timeClient.update();
  startTime2 = timeClient.getEpochTime();
  
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!Rtc.IsDateTimeValid()) {
    Rtc.SetDateTime(compiled);
  }

  if (!Rtc.GetIsRunning()) {
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();

  if (now < compiled) {
    Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
  } else if (now == compiled) {
  }

  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  
  inithttp();
  initjson();
  initstartTime();
  initdTime();
  initval();

  MDNS.begin(host);

  httpUpdater.setup(&httpServer);

  httpServer.onNotFound([](){
    if (!handleFileRead(httpServer.uri())) {
      httpServer.send(404, "text/plain", "404: Not Found");
    }
  });

  httpServer.on("/auth", HTTP_GET, [](){
    httpServer.sendHeader("Connection", "close");
    httpServer.send(200, "text/html", readAuthForm());
    authFormContent = "";
    isOnline = false;
  });

  httpServer.on("/configure", HTTP_POST, [](){
    String userarg = httpServer.arg("username");
    String passarg = httpServer.arg("password");

    DynamicJsonDocument json(600);
    deserializeJson(json, readConfigFile());

    String username = json["username"];
    String userpass = json["userpass"];
    
    httpServer.sendHeader("Connection", "close");
    
    if ((userarg == username) && (passarg == userpass)) {
      httpServer.send(200, "text/html", readConfigForm());
      isOnline = true;
    } else {
      httpServer.send(200, "text/html", "Login Failed.");
      isOnline = false;
    }
    
    configcontent = "";
  });

  httpServer.on("/submitconfig", HTTP_POST, [](){
    String networkid1 = httpServer.arg("networkid1");
    String networkid2 = httpServer.arg("networkid2");
    String ntpip = httpServer.arg("ntpip");
    String mcdapiendpoint = httpServer.arg("mcdapiendpoint");    
    String argwifi = httpServer.arg("wifi");
    String argpass = httpServer.arg("password");
    String argsubnet = httpServer.arg("subnet");
    String arghost = httpServer.arg("host");
    String arggatewaydnshost = httpServer.arg("gatewaydnshost");
    String argmid = httpServer.arg("mId");
    String argusername = httpServer.arg("username");
    String arguserpass = httpServer.arg("userpass");

    String json = "{\"mcdapiendpoint\": \""+mcdapiendpoint+"\", \"ntpip\": \""+ntpip+"\", \"networkid2\": "+networkid2+", \"networkid1\": "+networkid1+", \"username\": \""+argusername+"\", \"userpass\": \""+arguserpass+"\", \"host\": "+arghost+", \"mId\": "+argmid+", \"wifi\": \""+argwifi+"\", \"password\": \""+argpass+"\", \"subnet\": "+argsubnet+", \"gatewaydnshost\": "+arggatewaydnshost+"}";

    httpServer.send(200, "application/json", writeConfig(json));
  });

  httpServer.on("/errorpage", HTTP_GET, [](){
    httpServer.sendHeader("Connection", "close");
    httpServer.send(200, "text/html", readErrorForm());
    errorcontent = "";
  });

  httpServer.on("/getconfig", HTTP_GET, [](){
    httpServer.sendHeader("Connection", "close");
    httpServer.send(200, "application/json", isOnline ? readConfigFile() : "{\"success\": false, \"reason\": \"Please Login.\"}");
    configjson = "";
  });

  httpServer.on("/seeconfig", HTTP_GET, [](){
    httpServer.sendHeader("Connection", "close");
    httpServer.send(200, "application/json", readConfigFile());
    configjson = "";
  });
  
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
}

String readConfigFile() {
  File file = SPIFFS.open("/configs.json", "r");

  if (!file) {
    configjson = "{\"success\": false, \"reason\": \"Config file open failed.\"}";
  } else {

    configjson = "";

    while (file.available()) {
      char c = file.read();
      configjson += c;
    }
  }

  file.close();
  return configjson;
}

String readAuthForm() {
  File file = SPIFFS.open("/authform.html", "r");

  if (!file) {
    authFormContent = "Authentication Form Error.";
  } else {
    authFormContent = "";

    while (file.available()) {
      char c = file.read();
      authFormContent += c;
    }
  }

  file.close();
  return authFormContent;
}

String readConfigForm() {
  File file = SPIFFS.open("/configform.html", "r");

  if (!file) {
    configcontent = "Error occurred in the form";
  } else {

    configcontent;

    while (file.available()) {
      char c = file.read();
      configcontent += c;
    }
  }

  file.close();
  return configcontent;
}

String readErrorForm() {
  File file = SPIFFS.open("/errorpage.html", "r");

  if (!file) {
    errorcontent = "Error has occurred";
  } else {

    errorcontent;

    while (file.available()) {
      char c = file.read();
      errorcontent += c;
    }
  }

  file.close();
  return errorcontent;
}

String content;
void readConfig() {
  File file = SPIFFS.open("/configs.json", "r");

  if (!file) {
    Serial.println("Reading configs.json has failed.");
  } else {

    content = "";

    while (file.available()) {
      char c = file.read();
      content += c;
    }

    DynamicJsonDocument configs(600);
    deserializeJson(configs, content);
  
    networkid1conf = configs["networkid1"].as<int>();
    networkid2conf = configs["networkid2"].as<int>();
    subnetconf = configs["subnet"].as<int>();
    hostconf = configs["host"].as<int>();
    mcdapiendpointconf = configs["mcdapiendpoint"].as<String>();
    midconf = configs["mId"].as<int>();
    wificonf = configs["wifi"].as<String>();
    passwordconf = configs["password"].as<String>();
    gatewaydnshostconf = configs["gatewaydnshost"].as<int>();
    usernameconf = configs["username"].as<String>();
    userpassconf = configs["userpass"].as<String>();
    ntpipconf = configs["ntpip"].as<String>().c_str();

    NTPClient locntp(ntpUDP, "192.168.1.100", GMT_8);
    timeClient = locntp;

    WiFi.mode(WIFI_STA);
    WiFi.begin(wificonf, passwordconf);
    IPAddress ip(networkid1conf,networkid2conf,subnetconf,hostconf);
    IPAddress gateway(networkid1conf,networkid2conf,subnetconf,gatewaydnshostconf);
    IPAddress subnet(255,255,255,0);
    IPAddress dns(networkid1conf,networkid2conf,subnetconf,100);
    WiFi.config(ip,gateway,subnet,dns);
    
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      delay(500);
      ESP.restart();
    }
  }

  file.close();
}

String writeConfig(String confjson) {
  File file = SPIFFS.open("/configs.json", "w+");
  String ret;

  if (!file) {
    ret = "{\"success\": false, \"reason\": \"Configuration update failed.\"}";
  } else {
    int bytesWritten = file.print(confjson);

    if (bytesWritten > 0) {
      ret = "{\"success\": true}";
    } else {
      ret = "{\"success\": false, \"reason\": \"Configuration update failed.\"}";
    }
  }

  file.close();
  return ret;
}

bool handleFileRead(String path) {
  if (path.endsWith("/")) {
    path += "main.html";
  }

  String contentType = getContentType(path);

  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = httpServer.streamFile(file, contentType);

    file.close();
    return true;
  }

  return false;
}

String getContentType(String filename) {
  
  if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";    
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".svg")) {
    return "image/svg+xml";  
  }

  return "text/plain";
}

void inithttp() {  
	if (WiFi.status()== WL_CONNECTED) {
		http.begin(mcdapiendpointconf);
		http.addHeader("Content-Type", "application/x-www-form-urlencoded");

		char params[128];
		snprintf(params, sizeof params, "%s%d", "uId=2&h=3&fw=4&timestamp=5&mId=", midconf);
		statusCode = http.POST(params);
    
    response = http.getString();
    http.end();
	}
}

int success_json;

void initjson() {  
	while (!Serial) continue;

  DynamicJsonDocument doc(600);
  deserializeJson(doc, response);

  dTime = doc["dTime"];
  startTime = doc["startTime"];
  startTime2 = doc["startTime"];
  success_json = doc["success"];
}

void initval() {
	val0 = (dTime - startTime) *.15 ;
	val1 = val0 - 5;
	val2 = (dTime - startTime) *.10 ;
	val3 = val2 - 5;
}

void initdTime() {
  lcd.setCursor(0, 1);

  if (dTime > 0) {
    char ftime[32];
    sprintf(ftime, "End= %02d:%02d %02d/%02d", hour(dTime), minute(dTime), month(dTime), day(dTime));
    lcd.print(ftime);  
  } else {
    lcd.print("No Workorder    ");
  }
}

void initstartTime() {
    minutes = ((startTime % 3600) / 60);
    minutes = minutes + MM;
    hours = (startTime  % 86400L) / 3600;
	
    if(minutes > 59) {
  		hours = hours + HH + 1;
  		minutes = minutes - 60;
    } else {
		  hours = hours + HH;
    }

    seconds = (startTime % 60);
}

void loop() {
  long currEpochTime = timeClient.getEpochTime();
  
  sec_elapsed++;

  int dTimeLoc = 0;
  
  if (sec_elapsed > 30) {
    sec_elapsed = 0;

    //  query
    inithttp();

    //  if there is a response
    if (response.length() > 0) {

      //  ********
      //  parse json. get new dTime, startTime, success_json
      DynamicJsonDocument doc(600);
      deserializeJson(doc, response);    
      dTimeLoc = doc["dTime"];
      success_json = doc["success"];
      //  ********

      //  if there is no work order
      if (success_json < 1) {
        
          Serial.println(response);
          
          if (noWorkPassed > 10) {
              dTime = 0;
              startTime = 0;
              noWorkPassed = 0;
              initdTime();
          } else {
              noWorkPassed++;

              lcd.setCursor(0, 1);
              lcd.print("NoWork Shut 5min");
          }
      
      } else {  //  if there is work order, refresh dTime

          //  if dTime is new, refresh it
          if (dTime != dTimeLoc) {
              dTime = dTimeLoc;              
              initval();
              
              initstartTime();
          }
          initdTime();
      }
      
    } else {  //  empty response
      
      if (currEpochTime > dTime) {
          dTime = 0;
          startTime = 0;
          
          lcd.setCursor(0, 1);
          lcd.print("Server/WiFi Off ");
      } else {
          lcd.setCursor(0, 1);
          lcd.print("Connection Lost ");
      }
    }
  }

  httpServer.handleClient();
  MDNS.update();  
	ArduinoOTA.handle();

  int force_update_status;
  int update_status = timeClient.update();
  
  while(!update_status) {
    force_update_status = timeClient.forceUpdate();
    update_status = timeClient.update();
  }

	lcd.setCursor(0, 0);
	lcd.print("Now= ");
	lcd.print(timeClient.getFormattedTime());
  
	if ( inwork == 0 ) {
		digitalWrite(D5, LOW);
		digitalWrite(D3, LOW);
		digitalWrite(D4, HIGH);
		digitalWrite(D6, LOW);
		
		if (dTime < timeClient.getEpochTime()) {
			inwork = 0;
		} else if (startTime <= timeClient.getEpochTime()) {
			inwork = 1;
		}
	} else if ( inwork == 1 ) {
		digitalWrite(D5, HIGH);
		digitalWrite(D4, LOW);
		
		if (startTime <= timeClient.getEpochTime()) {			
			if (dTime - val0 <= timeClient.getEpochTime()) {
				digitalWrite(D3, HIGH);
				digitalWrite(D6, HIGH);
			}
			
			if (dTime - val1 <= timeClient.getEpochTime()) {
				digitalWrite(D6, LOW);
			}
			
			if (dTime - val2 <= timeClient.getEpochTime()) {
				digitalWrite(D6, HIGH);
			}
			
			if (dTime - val3 <= timeClient.getEpochTime()) {
				digitalWrite(D6, LOW);
			}

      if ((dTime - 60 <= timeClient.getEpochTime()) && (dTime-56 >= timeClient.getEpochTime())) {
        digitalWrite(D6, HIGH);
        Serial.println("WARNING");
      }

      //  4 seconds buzzer before turn off
			if (dTime - 4 <= timeClient.getEpochTime()) {
				digitalWrite(D6, HIGH);
        Serial.println("WARNING");
			}

      //  turn off
      if (dTime <= timeClient.getEpochTime()) {
        digitalWrite(D5, LOW);
        digitalWrite(D3, LOW);
        digitalWrite(D4, HIGH);
        digitalWrite(D6, LOW);
        inwork = 0;
      }
		}
	}

  delay(1000);
  startTime2++;
}

//https://www.teachmemicro.com/esp8266-spiffs-web-server-nodemcu/
//https://techtutorialsx.com/2019/05/28/esp8266-spiffs-writing-a-file/
//https://arduino-esp8266.readthedocs.io/en/2.5.2/filesystem.html#open
//https://links2004.github.io/Arduino/d3/d58/class_e_s_p8266_web_server.html
