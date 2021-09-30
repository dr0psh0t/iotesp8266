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

/*  north */
int hostId = 198;
int mId = 100071;
const char* ntpIp = "192.168.1.100";
//String api = "http://192.168.1.30:8080/mcsa/IoTCheckWorkQueue";
String api = "http://192.168.1.150:8080/joborder/IoTCheckWorkQueue";
//String ssid = "wmdcDev";
//String password = "(===|===Dev===|===)";
String ssid = "Wifi_Er";
String password = "wmdcwifier";
//String ssid = "Wifi_Mf";
//String password = "wmdcwifimf";
//String ssid = "wifi_mf2";
//String password = "wmdc_1959";

/* central
int hostId = 43;
int mId = 200033;
const char* ntpIp = "192.168.1.149";
String api = "http://192.168.1.149:8080/joborder/IoTCheckWorkQueue";
String ssid = "Wifi_Central";
String password = "(---WifiCentral---)";
 */

/* south 
int hostId = 49;
int mId = 3000113;
const char* ntpIp = "192.168.2.99";
String api = "http://192.168.2.99:8080/joborder/IoTCheckWorkQueue";
String ssid = "Wifi_South";
String password = "(---WifiSouth---)";
 */

/* bohol 
int hostId = 51;
int mId = 800038;
const char* ntpIp = "192.168.2.99";
String api = "http://192.168.2.155:8080/joborder/IoTCheckWorkQueue";
String ssid = "Wifi_Er";
String password = "wmdcwifier";
 */

const char* host = "daryll";
const char* hostName = "daryll";

int sec_elapsed = 0;
int success;
int httpResponseCode;
int inwork = 0;
char HH = 00;
char MM = 00;
char hours, minutes, seconds;
long dTime;
long startTime;
long val0;
long val1;
long val2;
long val3;
const int GMT_8 = 28800;
String response;
String configcontent;
String errorcontent;

const char* ipConf = "";
int midConf;
const char* wifiConf = "";
const char* passConf = "";
int subnetConf;
int hostConf;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
HTTPClient http;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpIp, GMT_8);
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
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  IPAddress ip(192,168,1,hostId);
  IPAddress gateway(192,168,1,100);
  IPAddress subnet(255,255,255,0);
  IPAddress dns(192,168,1,100);
  WiFi.config(ip, gateway, subnet,dns);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(500);
    ESP.restart();
  }

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount failed");  
  } else {
    Serial.println("SPIFFS Mount successful");
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
  
  lcd.init();
  lcd.backlight();
  
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
  } else if (now > compiled) {} else if (now == compiled) {}

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

  httpServer.on("/configure", HTTP_GET, [](){
    httpServer.sendHeader("Connection", "close");
    httpServer.send(200, "text/html", readConfigForm());
    configcontent = "";
  });

  httpServer.on("/submitconfig", HTTP_POST, [](){
    String argwifi = httpServer.arg("wifi");
    String argpass = httpServer.arg("password");
    String argsubnet = httpServer.arg("subnet");
    String arghost = httpServer.arg("host");
    String arggatewaydnshost = httpServer.arg("gatewaydnshost");
    String argmid = httpServer.arg("mId");
    
    Serial.println(argwifi);
    Serial.println(argpass);
    Serial.println(argsubnet);
    Serial.println(arghost);
    Serial.println(arggatewaydnshost);
    Serial.println(argmid);

    httpServer.send(200, "text/html", "Submit Successful");
  });

  httpServer.on("/errorpage", HTTP_GET, [](){
    httpServer.sendHeader("Connection", "close");
    httpServer.send(200, "text/html", readErrorForm());
    errorcontent = "";
  });
  
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
}

String readConfigForm() {
  File file = SPIFFS.open("/configform.html", "r");

  if (!file) {
    Serial.println("file open failed");
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
    Serial.println("file open failed");
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
  File file = SPIFFS.open("/configs.txt", "r");

  if (!file) {
    Serial.println("file open failed");
  } else {

    content;

    while (file.available()) {
      char c = file.read();
      content += c;
    }

    DynamicJsonDocument configs(600);
    deserializeJson(configs, content);
  
    ipConf = configs["ip"];
    midConf = configs["mId"];
    wifiConf = configs["wifi"];
    passConf = configs["password"];
    subnetConf = configs["subnet"];
    hostConf = configs["host"];
  }

  file.close();
}

bool handleFileRead(String path) {
  Serial.println("handleFileRead: "+path);

  if (path.endsWith("/")) {
    path += "main.html";
  }

  String contentType = getContentType(path);

  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = httpServer.streamFile(file, contentType);

    Serial.println(file.size());

    file.close();
    return true;
  }

  Serial.println("\tFile Not Found");
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
	if(WiFi.status()== WL_CONNECTED) {
		http.begin(api);
		http.addHeader("Content-Type", "application/x-www-form-urlencoded");

		char params[128];
		snprintf(params, sizeof params, "%s%d", "uId=2&h=3&fw=4&timestamp=5&mId=", mId);
		httpResponseCode = http.POST(params);

		if (httpResponseCode > 0) {
			response = http.getString();
			http.end();
		}
	}
}

int success_json;

void initjson() {  
	while (!Serial) continue;

  DynamicJsonDocument doc(600);
  deserializeJson(doc, response);

  dTime = doc["dTime"];
  startTime = doc["startTime"];
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
    lcd.print("No Workorder");
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
  sec_elapsed++;

  if (sec_elapsed > 29) {
    sec_elapsed = 0;

    inithttp();

    if (!response.length() == 0) {
      initjson();

      if (success_json != 1) {
        dTime = 0;
        startTime = 0;
      }

      initval();
      initdTime();
      initstartTime();
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
			
			if(dTime - val1 <= timeClient.getEpochTime()) {
				digitalWrite(D6, LOW);
			}
			
			if(dTime - val2 <= timeClient.getEpochTime()) {
				digitalWrite(D6, HIGH);
			}
			
			if(dTime - val3 <= timeClient.getEpochTime()) {
				digitalWrite(D6, LOW);
			}
			
			if(dTime - 4 <= timeClient.getEpochTime()) {
				digitalWrite(D6, HIGH);
			}
			
     if (dTime <= timeClient.getEpochTime()) {
				digitalWrite(D5, LOW);
				digitalWrite(D3, LOW);
				digitalWrite(D4, HIGH);
				digitalWrite(D6, LOW);
				inwork = 0;
			}
		}
	}

  Serial.println();
  delay(2000);
}
