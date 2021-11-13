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

const char* host = "none";
const char* hostName = "none";
int hostId = 82;
int mId = 100071;

//  Dev
//const char* ssid = "wmdcDev";
//const char* password = "(===|===Dev===|===)";

// ER Area
const char* ssid = "Wifi_Er";
const char* password = "wmdcwifier";

// MF Area
//const char* ssid = "Wifi_Mf";
//const char* password = "wmdcwifimf";

// Finance Area
//const char* ssid = "wifi_mf2";
//const char* password = "wmdc_1959";

int sec_elapsed = 0;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

HTTPClient http;  
const char* reason; int success;
int httpResponseCode;
long dTime;
long startTime;
long val0;
long val1;
long val2;
long val3;
String response;
int inwork = 0; //  0 = no work, 1 = in work
char HH = 00;
char MM = 00;
char hours, minutes, seconds;

const int GMT_8 = 28800;
WiFiUDP ntpUDP;
// By default 'pool.ntp.org' is used with 60 seconds update interval and
//NTPClient timeClient(ntpUDP, "time.nist.gov", GMT_8);
NTPClient timeClient(ntpUDP, "192.168.1.100", GMT_8);

RtcDS3231<TwoWire> Rtc(Wire);
#define countof(a) (sizeof(a) / sizeof(a[0]))

LiquidCrystal_I2C lcd(0x27,16,2);

const char* serverIndex = "";

void initLCD() {
	lcd.init();// initialize the lcd
	lcd.backlight();
}

void initWiFi() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  IPAddress ip(192,168,1,hostId);
  IPAddress gateway(192,168,1,100);
  IPAddress subnet(255,255,255,0);
  IPAddress dns(192,168,1,100);
  WiFi.config(ip, gateway, subnet,dns);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(500);
    ESP.restart();
  }

  ArduinoOTA.setHostname(hostName);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.println("WiFi connected to!!!");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void printDateTime(const RtcDateTime& dt) {
  char datestring[20];
  snprintf_P(datestring, countof(datestring), PSTR("%02u/%02u/%04u"), dt.Month(), dt.Day(), dt.Year());
  Serial.println(datestring);
}

void initRtc() {
  Serial.print("compiled: ");

  //--------RTC SETUP ------------
  //if you are using ESP-01 then uncomment the line below to reset the pins to
  //the available pins for SDA, SCL
  //Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) {
    //Common Cuases:
    //1) first time you ran and the device wasn't running yet
    //2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");
    //following line sets the RTC to the date & time this sketch was compiled
    //it will also reset the valid flag internally unless the Rtc device is
    //having an issue

    Rtc.SetDateTime(compiled);
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();

  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
    Serial.println("RTC is newer than compile time. (this is expected)");
  } else if (now == compiled) {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  // never assume the Rtc was last configured by you, so
  // just clear them to your needed state

  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
}

void inithttp() {
  Serial.println("=== inithttp() ===");
  
	if(WiFi.status()== WL_CONNECTED) {   //Check WiFi connection status
		http.begin("http://192.168.1.150:8080/joborder/IoTCheckWorkQueue");		//Specify destination for HTTP request
		//http.begin("http://192.168.1.30:8080/mcsa/IoTCheckWorkQueue");		//Specify destination for HTTP request
    //http.begin("http://192.168.1.150:8080/dhijo/IoTCheckWorkQueue");    //Specify destination for HTTP request
		//http.begin("http://58.69.126.27:3316/joborder/IoTCheckWorkQueue");    //Specify destination for HTTP request
		http.addHeader("Content-Type", "application/x-www-form-urlencoded");	//Specify content-type header

		char params[128];
		snprintf(params, sizeof params, "%s%d", "uId=2&h=3&fw=4&timestamp=5&mId=", mId);
		httpResponseCode = http.POST(params);

		if (httpResponseCode > 0) {
			response = http.getString();		//	Get the response to the request
			Serial.print("httpresponsecode= ");
			Serial.println(httpResponseCode);	//	Print return code
			Serial.print("response= ");
			Serial.println(response);			//	Print request answer
			http.end();	//	Free resources
		} else {
			Serial.println("Error in WiFi connection");
		}
	}

 Serial.println();
}

int success_json;

void initjson() {
	Serial.println("=== initjson() ===");
  
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
	Serial.println("=== initstartTime() ===");
	Serial.print("startTime= ");

	// print the hour, minute and second:
    minutes = ((startTime % 3600) / 60);
    minutes = minutes + MM;	//	Add UTC Time Zone    
    hours = (startTime  % 86400L) / 3600;
	
    if(minutes > 59) {
		hours = hours + HH + 1; //	Add UTC Time Zone  
		minutes = minutes - 60;
    } else {
		hours = hours + HH;
    }
	
    if ( hours < 10 ) {
		//	In the first 10 seconds of each minute, we'll want a leading '0'
		Serial.print('0');
    }
	
    Serial.print(hours,DEC);	//	print the hour (86400 equals secs per day)
    Serial.print(':');
	
    if ( minutes < 10 ) {
		//	In the first 10 minutes of each hour, we'll want a leading '0'
		Serial.print('0');
    }
	
    Serial.print(minutes,DEC);	//	print the minute (3600 equals secs per minute)
    Serial.print(':');
    seconds = (startTime % 60);
	
    if ( seconds < 10 ) {
		//	In the first 10 seconds of each minute, we'll want a leading '0'
		Serial.print('0');
    }
	
    Serial.println(seconds,DEC); // print the second
    Serial.println();
}

void setup() {
	Serial.begin(115200);
	pinMode(D5, OUTPUT);// green
	pinMode(D3, OUTPUT);//Red
	pinMode(D4, OUTPUT);//orange
	pinMode(D6, OUTPUT);//buzzer
	initWiFi();
	timeClient.begin();
	initLCD();
	initRtc();
	inithttp();
	initjson();
	initstartTime();
	initdTime();
	initval();

  MDNS.begin(host);

  httpUpdater.setup(&httpServer);
  
  httpServer.on("/", HTTP_GET, [](){
    httpServer.sendHeader("Connection", "close");
    httpServer.send(200, "text/html", serverIndex);
  });
  
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
}

void loop() {
  sec_elapsed++;
  Serial.print("seconds: ");
  Serial.println(sec_elapsed);

  if (sec_elapsed > 29) {
    sec_elapsed = 0;

    //  query and check server response by calling "inithttp()".
    inithttp();

    /*  if "response" is empty, the server lags or is unresponsive. 
     *    the work will still continue and will wait for the next 30 seconds to query.
     *  if its not empty, it gets the response and will proceed in the if block.
     */
    Serial.print("response: ");
    Serial.println(response);
    if (!response.length() == 0) {
      initjson();

      /*  query succeeded. a response is returned in a form of json string.
       *  check the json: 
       *  if success is 1, workorder is currently running,
       *    get the startTime and dTime and refresh-display those times in lcd.
       *  if success is not 1, there is no workorder or no machine assigned.
       *    assign 0 value to dTime and startTime and refresh-display those times in lcd.
       */
      Serial.print("success_json: ");
      Serial.println(success_json);

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

  Serial.println("=== inside loop() ===");

  int force_update_status;
  int update_status = timeClient.update();
  Serial.print("out timeClient.update() returns "); 
  Serial.println(update_status);
  
  while(!update_status){
    force_update_status = timeClient.forceUpdate();
    Serial.print("in force_update_status= ");
    Serial.println(force_update_status);

    update_status = timeClient.update();
    Serial.print("in timeClient.update() returns "); 
    Serial.println(update_status);
  }  
  
  //  returns 1 if timeClient update is succesful. 0 if not
  Serial.println("timeClient.getFormattedTime()= "+timeClient.getFormattedTime());
  Serial.print("timeClient.getEpochTime()= ");
  Serial.print(timeClient.getEpochTime());  
  Serial.println();
  
	RtcDateTime now = Rtc.GetDateTime();

  Serial.print("dTime= ");
  Serial.println(dTime);
  Serial.print("now.Epoch32Time()= ");
  Serial.println(now.Epoch32Time());
  
	lcd.setCursor(0, 0);
	lcd.print("Now= ");
	lcd.print(timeClient.getFormattedTime());

  Serial.print("Date= ");
	printDateTime(now);
	
	//	Serial.println(int64String(now.TotalSeconds64()));
	//	Serial.println( String(now.Epoch32Time()) + " - " + String(deadlineTime));
	Serial.println();
	
	//Serial.println(int64String(now.Epoch64Time()));
  
	if ( inwork == 0 ) {
		digitalWrite(D5, LOW);
		digitalWrite(D3, LOW);
		digitalWrite(D4, HIGH);
		digitalWrite(D6, LOW);
		
		//	lcd.setCursor(0, 0);
		//  lcd.print("No Work");
		Serial.println("---No Work, checking server");
		
		if (dTime < timeClient.getEpochTime()) {
			Serial.println("---no work, waiting for startTime..."); 
			inwork = 0;
		} else if (startTime <= timeClient.getEpochTime()) {
			Serial.println("---Work Found, setting work to true");
			inwork = 1;
		}
	} else if ( inwork == 1 ) {
	
		Serial.println("---Work in process");
		digitalWrite(D5, HIGH);
		digitalWrite(D4, LOW);
		
		if (startTime <= timeClient.getEpochTime()) {
			Serial.println("---Still in  work, waiting for time...");
			
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
			
			//if (dTime <= now.Epoch32Time()) {
     if (dTime <= timeClient.getEpochTime()) {
				Serial.println("---Deadline time has passed, turning off work...");
				digitalWrite(D5, LOW);
				digitalWrite(D3, LOW);
				digitalWrite(D4, HIGH);
				digitalWrite(D6, LOW);
				inwork = 0;
			}
		} else {
		  Serial.println("---Unknown");
		}
	}

  Serial.println();
  delay(1000);
}
