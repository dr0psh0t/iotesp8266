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

int hostId = 210;
int mId = 208;

const char* ssid = "wmdcDev";
const char* password = "(===|===Dev===|===)";

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

void sendRequest() {
  
	if(WiFi.status()== WL_CONNECTED) {   //Check WiFi connection status
		http.begin("http://192.168.1.150:8080/joborder/IoTCheckWorkQueue");		//Specify destination for HTTP request
		http.addHeader("Content-Type", "application/x-www-form-urlencoded");	//Specify content-type header
    http.addHeader("Content-Length", "2000");

		char params[128];
		snprintf(params, sizeof params, "%s%d", "uId=2&h=3&fw=4&timestamp=5&mId=1000", mId);
		httpResponseCode = http.POST(params);    
    Serial.println(http.getString());

    /*
		if (httpResponseCode > 0) {
			response = http.getString();
			Serial.print("httpresponsecode= ");
			Serial.println(httpResponseCode);
			Serial.print("response= ");
			Serial.println(response);
			http.end();

		} else {
			Serial.println("Error in WiFi connection");
		}*/
	}

 Serial.println();
}

void setup() {
	Serial.begin(115200);  
	initWiFi();
}

void loop() {

  delay(100);
  sendRequest();
}
