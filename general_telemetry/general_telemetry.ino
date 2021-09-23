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
#include "ThingsBoard.h"

#define TOKEN "Gjy4KDQXqYcVtj9sZ6bh"
char thingsboardServer[] = "192.168.1.192";

WiFiClient wifiClient;
ThingsBoard tb(wifiClient);

int status = WL_IDLE_STATUS;
unsigned long lastSend;

const char* host = "none";
const char* hostName = "none";
int hostId = 205;
int mId = 148;

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
const char* reason; 
int success;
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

const char* startWork;
const char* endWork;
const char* joInfo;
 
String startOfWork;
String endOfWork;
int mcdId;

const int GMT_8 = 28800;
WiFiUDP ntpUDP;
// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
//NTPClient timeClient(ntpUDP, "time.nist.gov", GMT_8);
NTPClient timeClient(ntpUDP, "192.168.1.150", GMT_8);

RtcDS3231<TwoWire> Rtc(Wire);
#define countof(a) (sizeof(a) / sizeof(a[0]))

LiquidCrystal_I2C lcd(0x27,16,2);

const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<h2>ESP8266 Login</h2>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<input name=userid placeholder='User ID'></br>"
  "<input name=pwd placeholder=Password type=Password></br></br>"
  "<input type='file' name='update'>"
  "<input type='submit' value='Update'>"
"</form>"
"<div id='prg'>progress: 0%</div>"
"<script>"
  "$('form').submit(function(e){"

    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"

    "if(form.userid.value=='admin' && form.pwd.value=='admin') {"
      "$.ajax({"
        "url: '/update',"
        "type: 'POST',"
        "data: data,"
        "contentType: false,"
        "processData:false,"
        "xhr: function() {"
          "var xhr = new window.XMLHttpRequest();"
          "xhr.upload.addEventListener('progress', function(evt) {"
            "if (evt.lengthComputable) {"
              "var per = evt.loaded / evt.total;"
              "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
            "}"
          "}, false);"
          "return xhr;"
        "},"
        "success:function(d, s){"
          "console.log('success!')"
        "},"
        "error: function (a, b, c) {"
        "}"
      "});"
    "} else {"
      "alert('Error Password or Username')"
    "}"
  "});"
"</script>";

void initLCD() {
	//	initialize the lcd
	lcd.init();
	lcd.backlight();
}

void initWiFi() {
	Serial.println("Booting");
	
	Serial.begin(115200);
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
		
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed");
		} else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed");
		} else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed");
		} else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed");
		} else if (error == OTA_END_ERROR) {
			Serial.println("End Failed");
		}
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
}

void initRtc() {
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

	//	Check WiFi connection status
	if (WiFi.status() == WL_CONNECTED) {
		
		http.begin("http://192.168.1.150:8080/joborder/IoTCheckWorkQueue");
		//http.begin("http://192.168.1.30:8080/mcsa/IoTCheckWorkQueue");
		http.addHeader("Content-Type", "application/x-www-form-urlencoded");

		char params[128];
		snprintf(params, sizeof params, "%s%d", "uId=2&h=3&fw=4&timestamp=5&mId=1000", mId);
		httpResponseCode = http.POST(params);

		if (httpResponseCode > 0) {
			response = http.getString();
			Serial.print("httpresponsecode= ");
			Serial.println(httpResponseCode);
			Serial.print("response= ");
			Serial.println(response);
			http.end(); //  Free resources
		} else {
			Serial.println("Error in WiFi connection");
		}
	}

	Serial.println();
}

void initjson() {
	Serial.println("=== initjson() ===");

	while (!Serial) continue;

	DynamicJsonDocument doc(600);
	deserializeJson(doc, response);

	success = doc["success"];

	//	no error
	if (success > 0) {
		dTime = doc["dTime"];
		startTime = doc["startTime"];
		joInfo = doc["joInfo"];
		mcdId = doc["mId"];

		tb.sendTelemetryString("JO Info", joInfo);
		tb.sendTelemetryString("Reason", "");
	} else {
		reason = doc["reason"];
		tb.sendTelemetryString("JO Info", "");
		tb.sendTelemetryString("Reason", reason);
	}

	tb.loop();
	
	Serial.println("=== initjson() ===");
}

void initval() {  
	val0 = (dTime - startTime) * 0.15 ;
	val1 = val0 - 5;
	val2 = (dTime - startTime) * 0.10 ;
	val3 = val2 - 5;
}

void initdTime() {
	Serial.println("=== initdTime() ===");

	//  print the hour, minute and second:
	minutes = ((dTime % 3600) / 60);
	minutes = minutes + MM; //Add UTC Time Zone
	hours = (dTime  % 86400L) / 3600;

	if (minutes > 59) {
		hours = hours + HH + 1; //Add UTC Time Zone  
		minutes = minutes - 60;
	} else {
		hours = hours + HH;
	}

	lcd.setCursor(0, 1);
	lcd.print("DTime:");

	if ( hours < 10 ) {
		//	In the first 10 seconds of each minute, we'll want a leading '0'
		Serial.print('0');
		lcd.print('0');
	}

	Serial.print(hours,DEC);  //  print the hour (86400 equals secs per day)
	Serial.print(':');
	lcd.print(hours,DEC);
	lcd.print(':');

	if ( minutes < 10 ) {
		//  In the first 10 minutes of each hour, we'll want a leading '0'
		Serial.print('0'); 
		lcd.print('0');
	}

	Serial.print(minutes,DEC);  //  print the minute (3600 equals secs per minute)
	Serial.print(':');
	lcd.print(minutes,DEC);
	lcd.print(':');
	seconds = (dTime % 60);

	if ( seconds < 10 ) {
		//  In the first 10 seconds of each minute, we'll want a leading '0'
		Serial.print('0');
		lcd.print('0');
	}  

	Serial.println(seconds,DEC);  //  print the second
	lcd.print(seconds,DEC);
	Serial.println();
}

void initstartTime() {
	Serial.println("=== initstartTime() ===");
	Serial.print("startTime= ");

	//	print the hour, minute and second:
    minutes = ((startTime % 3600) / 60);
    minutes = minutes + MM; //  Add UTC Time Zone    
    hours = (startTime  % 86400L) / 3600;
  
    if (minutes > 59) {
		hours = hours + HH + 1; //  Add UTC Time Zone  
		minutes = minutes - 60;
    } else {
		hours = hours + HH;
    }
  
    if ( hours < 10 ) {
		//  In the first 10 seconds of each minute, we'll want a leading '0'
		Serial.print('0');
    }
  
    Serial.print(hours,DEC);  //  print the hour (86400 equals secs per day)
    Serial.print(':');
  
    if ( minutes < 10 ) {
		//  In the first 10 minutes of each hour, we'll want a leading '0'
		Serial.print('0');
    }
  
    Serial.print(minutes,DEC);  //  print the minute (3600 equals secs per minute)
    Serial.print(':');
    seconds = (startTime % 60);
  
    if ( seconds < 10 ) {
		//  In the first 10 seconds of each minute, we'll want a leading '0'
		Serial.print('0');
    }
  
    Serial.println(seconds,DEC); // print the second
    Serial.println();
}

void setup() {
	Serial.begin(115200);

	delay(10);

	pinMode(D5, OUTPUT);//green
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

	httpServer.on("/", HTTP_GET, []() {
		httpServer.sendHeader("Connection", "close");
		httpServer.send(200, "text/html", serverIndex);
	});

	httpServer.begin();

	MDNS.addService("http", "tcp", 80);

	lastSend = 0;
}

String mcdIdStr;

void loop() {
	
	sec_elapsed++;
	Serial.print("seconds: ");
	Serial.println(sec_elapsed);

	if (sec_elapsed > 29) {
		sec_elapsed = 0;

		//  query and check server response by calling "inithttp()".
		inithttp();

		//  if "response" is empty, the server lags or is unresponsive. 
		//  the work will still continue and will wait for the next 30 seconds to query.
		//  if its not empty, it gets the response and will proceed in the if block.
		Serial.print("response: ");
		Serial.println(response);

		if (!response.length() == 0) {
			initjson();

			//  query succeeded. a response is returned in a form of json string.
			//  check the json: 
			//  if success is 1, workorder is currently running,
			//    get the startTime and dTime and refresh-display those times in lcd.
			//  if success is not 1, there is no workorder or no machine assigned.
			//    assign 0 value to dTime and startTime and refresh-display those times in lcd.
			
			Serial.print("success: ");
			Serial.println(success);

			if (success != 1) {
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

	while(!update_status) {
		force_update_status = timeClient.forceUpdate();
		Serial.print("in force_update_status= ");
		Serial.println(force_update_status);

		update_status = timeClient.update();
		Serial.print("in timeClient.update() returns "); 
		Serial.println(update_status);
	}

	//startOfWork = timeClient.getFormattedTime();

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
	lcd.print("CTime:");
	lcd.print(timeClient.getFormattedTime());

	Serial.print("Date= ");
	printDateTime(now);

	String monthStr(now.Month());
	String dayStr(now.Day());
	String yearStr(now.Year());
	mcdIdStr = String(mcdId);

	//startOfWork += " "+yearStr+"-"+monthStr+"-"+dayStr;

	//  startTime
	char startTimeChar[32];
	sprintf(startTimeChar, "%02d:%02d:%02d %02d-%02d-%02d", hour(startTime), minute(startTime), second(startTime), year(startTime), month(startTime), day(startTime));
	startWork = startTimeChar;  //  char array to char pointer

	//  dTime
	char dTimeChar[32];
	sprintf(dTimeChar, "%02d:%02d:%02d %02d-%02d-%02d", hour(dTime), minute(dTime), second(dTime), year(dTime), month(dTime), day(dTime));
	endWork = dTimeChar;    //  char array to char pointer
	//String buffStr(buff);
	//endOfWork = buffStr;
	//endWork = endOfWork.c_str();  //  string to char pointer

	//  Serial.println(int64String(now.TotalSeconds64()));
	//  Serial.println( String(now.Epoch32Time()) + " - " + String(deadlineTime));
	Serial.println();

	//Serial.println(int64String(now.Epoch64Time()));
  
	if ( inwork == 0 ) {
		digitalWrite(D5, LOW);
		digitalWrite(D3, LOW);
		digitalWrite(D4, HIGH);
		digitalWrite(D6, LOW);

		//  lcd.setCursor(0, 0);
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
	
	/*
	if (!tb.connected()) {	//	if not connected to TB
		if (tb.connect(thingsboardServer, TOKEN)) {
			Serial.println("You are now connected to TB.");
		} else {
			Serial.println("Failed to connect to TB.");
		}
	} else {	//	if connected to TB
		Serial.println("Connected to TB.");
	}*/
	
	if (sec_elapsed % 10 == 0) {
		if (tb.connected()) {	//	if connected to TB
			Serial.println("Connected to TB.");
		
			if (success > 0) {
				tb.sendTelemetryString("Start", startWork);
				tb.sendTelemetryString("End", endWork);
			} else {
				tb.sendTelemetryString("Start", "");
				tb.sendTelemetryString("End", "");
			}
			
			tb.sendTelemetryInt("MCD ID", mcdId);
			tb.loop();
			
		} else {	//	not connected to TB
			Serial.println("Attempting to connect and checking status...");
			
			if (tb.connect(thingsboardServer, TOKEN)) {		//	attempting connection and check status		
				Serial.println("Connection to TB succeeded.");
				
				if (success > 0) {
					tb.sendTelemetryString("Start", startWork);
					tb.sendTelemetryString("End", endWork);
				} else {
					tb.sendTelemetryString("Start", "");
					tb.sendTelemetryString("End", "");
				}

				tb.sendTelemetryInt("MCD ID", mcdId);
				tb.loop();
				
			} else {	//	failed TB connection despite reconnection.
				Serial.println("Failed TB reconnection.");
			}
		}
	}
	
	/*
	reconnect();
	
	if (sec_elapsed % 10 == 0) {
			
		Serial.println(">>>> Connected to ThingsBoard <<<<");
		
		if (success > 0) {
			tb.sendTelemetryString("Start", startWork);
			tb.sendTelemetryString("End", endWork);
		} else {
			tb.sendTelemetryString("Start", "");
			tb.sendTelemetryString("End", "");
		}
		
		tb.sendTelemetryInt("MCD ID", mcdId);
		tb.loop();
	}*/
}

void reconnect() {
	
	/*
	if (!tb.connected()) {
		Serial.println(">>>>>>>>>> Not Connected to ThingsBoard <<<<<<<<<<<");
		
		status = WiFi.status();
		
		if ( status != WL_CONNECTED) {
			
			Serial.println(">>>> Not Connected to WIFI <<<<");
			
			//WiFi.begin(ssid, password);			
			//while (WiFi.status() != WL_CONNECTED) {
				//Serial.print("Looping until connected...");
			//}
			
		} else {
			Serial.println(">>>> Connected to WIFI <<<<");
		}
		
		if ( tb.connect(thingsboardServer, TOKEN) ) {
			Serial.println(">>>> Successfully connected to ThingsBoard <<<<");
		} else {
			Serial.println(">>>> Failed to connect to ThingsBoard <<<<");
		}
		
	} else {
		Serial.println(">>>>>>>>>> Connected to ThingsBoard <<<<<<<<<<<");
	}
	*/	
	
	
	
	/*
	// Loop until we're reconnected
	while (!tb.connected()) {
		status = WiFi.status();
		
		if ( status != WL_CONNECTED) {
			WiFi.begin(ssid, password);
			
			while (WiFi.status() != WL_CONNECTED) {
				delay(500);
				Serial.print(".");
			}
			
			Serial.println("Connected to AP");
		}
		
		Serial.print("Connecting to ThingsBoard node ...");
		
		if ( tb.connect(thingsboardServer, TOKEN) ) {
			Serial.println( "[DONE]" );
		} else {
			Serial.print( "[FAILED]" );
			Serial.println( " : retrying in 5 seconds]" );
			// Wait 5 seconds before retrying
			delay( 2000 );
		}
	}*/
}
