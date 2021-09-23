#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>1
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
#include <ArduinoJson.h>

//  change only here
long dTime = 1614785719;
long startTime = 1614784519;
int hostId = 67;

const char* host = "webupdate";
//const char* ssid = "wmdcDev";
//const char* password = "(===|===Dev===|===)";

const char* ssid = "Wifi_Er";
const char* password = "wmdcwifier";

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

HTTPClient http;
const char* reason; int success;
int httpResponseCode;
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
// no offset
NTPClient timeClient(ntpUDP, "time.nist.gov", GMT_8);
//NTPClient timeClient(ntpUDP, "192.168.1.150", GMT_8);

RtcDS3231<TwoWire> Rtc(Wire);
#define countof(a) (sizeof(a) / sizeof(a[0]))

LiquidCrystal_I2C lcd(0x27, 16, 2);

/* Login page */
String loginIndex =
  "<form name=loginForm>"
  "<h1>ESP32 Login</h1>"
  "<input name=userid placeholder='User ID'> "
  "<input name=pwd placeholder=Password type=Password> "
  "<input type=submit onclick=check(this.form) class=btn value=Login></form>"
  "<script>"
  "function check(form) {"
  "if(form.userid.value=='admin' && form.pwd.value=='admin')"
  "{window.open('/serverIndex')}"
  "else"
  "{alert('Error Password or Username')}"
  "}"
  "</script>";

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
  lcd.init();  // initialize the lcd
  lcd.backlight();
}

void initWiFi() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  IPAddress ip(192, 168, 1, hostId);
  IPAddress gateway(192, 168, 1, 100);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(192, 168, 1, 100);
  WiFi.config(ip, gateway, subnet, dns);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(500);
    ESP.restart();
  }

  //ArduinoOTA.setPassword((const char *)"123");

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

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

/*
  void initNtp() {
  timeClient.begin();
  } */

void printDateTime(const RtcDateTime& dt) {
  char datestring[20];

  /*
    snprintf_P(datestring, countof(datestring), PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
    dt.Month(),
    dt.Day(),
    dt.Year(),
    dt.Hour(),
    dt.Minute(),
    dt.Second());*/

  snprintf_P(datestring, countof(datestring), PSTR("%02u/%02u/%04u"),
             dt.Month(), dt.Day(), dt.Year());

  Serial.println(datestring);
}

void initRtc() {

  Serial.print("compiled: ");

  //--------RTC SETUP ------------
  // if you are using ESP-01 then uncomment the line below to reset the pins to
  // the available pins for SDA, SCL
  // Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) {
    // Common Cuases:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");

    // following line sets the RTC to the date & time this sketch was compiled
    // it will also reset the valid flag internally unless the Rtc device is
    // having an issue

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

// the setup function runs once when you press reset or power the board

void initval() {
  val0 = (dTime - startTime) * .15 ;
  val1 = val0 - 5;
  val2 = (dTime - startTime) * .10 ;
  val3 = val2 - 5;

  /*
    Serial.println("val");
    Serial.println("+++++++");
    Serial.println(val0);
    Serial.println(val1);
    Serial.println(val2);
    Serial.println(val3);
    Serial.println("+++++++");
  */
}

void initdTime() {
  Serial.println("=== initdTime() ===");
  Serial.print("dTime= ");

  // print the hour, minute and second:
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
    // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.print('0');
    lcd.print('0');
  }

  Serial.print(hours, DEC); // print the hour (86400 equals secs per day)
  Serial.print(':');
  lcd.print(hours, DEC);
  lcd.print(':');

  if ( minutes < 10 ) {
    // In the first 10 minutes of each hour, we'll want a leading '0'
    Serial.print('0');
    lcd.print('0');
  }

  Serial.print(minutes, DEC); // print the minute (3600 equals secs per minute)
  Serial.print(':');
  lcd.print(minutes, DEC);
  lcd.print(':');
  seconds = (dTime % 60);

  if ( seconds < 10 ) {
    // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.print('0');
    lcd.print('0');
  }

  Serial.println(seconds, DEC); // print the second
  lcd.print(seconds, DEC);
}

void initstartTime() {
  Serial.println("=== initstartTime() ===");
  Serial.print("startTime= ");

  // print the hour, minute and second:
  minutes = ((startTime % 3600) / 60);
  minutes = minutes + MM; //Add UTC Time Zone
  hours = (startTime  % 86400L) / 3600;

  if (minutes > 59) {
    hours = hours + HH + 1; //Add UTC Time Zone
    minutes = minutes - 60;
  } else {
    hours = hours + HH;
  }

  if ( hours < 10 ) {
    // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.print('0');
  }

  Serial.print(hours, DEC); // print the hour (86400 equals secs per day)
  Serial.print(':');

  if ( minutes < 10 ) {
    // In the first 10 minutes of each hour, we'll want a leading '0'
    Serial.print('0');
  }

  Serial.print(minutes, DEC); // print the minute (3600 equals secs per minute)
  Serial.print(':');
  seconds = (startTime % 60);

  if ( seconds < 10 ) {
    // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.print('0');
  }

  Serial.println(seconds, DEC); // print the second
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
  initstartTime();
  initdTime();
  initval();

  MDNS.begin(host);

  httpUpdater.setup(&httpServer);

  httpServer.on("/", HTTP_GET, []() {
    httpServer.sendHeader("Connection", "close");
    httpServer.send(200, "text/html", serverIndex);
  });

  /*
    httpServer.on("/serverIndex", HTTP_GET, [](){
      httpServer.sendHeader("Connection", "close");
      httpServer.send(200, "text/html", serverIndex);
    });*/

  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
}

void loop() {
  {
    initval();
    initdTime();
    initstartTime();
  }

  httpServer.handleClient();
  MDNS.update();

  ArduinoOTA.handle();

  Serial.println("=== inside loop() ===");

  int force_update_status;
  int update_status = timeClient.update();
  Serial.print("out timeClient.update() returns ");
  Serial.println(update_status);

  while (!update_status) {
    force_update_status = timeClient.forceUpdate();
    Serial.print("in force_update_status= ");
    Serial.println(force_update_status);

    update_status = timeClient.update();
    Serial.print("in timeClient.update() returns ");
    Serial.println(update_status);
  }

  //  returns 1 if timeClient update is succesful. 0 if not
  Serial.println("timeClient.getFormattedTime()= " + timeClient.getFormattedTime());
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
    if (dTime < now.Epoch32Time()) {
      Serial.println("---no work, waiting for startTime...");
      inwork = 0;
    } else if (startTime <= now.Epoch32Time()) {
      Serial.println("---Work Found, setting work to true");
      inwork = 1;
    }
  } else if ( inwork == 1 ) {
    //  lcd.setCursor(0, 0);
    //  lcd.print("Working");
    Serial.println("---Work in process");
    digitalWrite(D5, HIGH);
    digitalWrite(D4, LOW);
    Serial.println( String(now.Epoch32Time()) + " - " + String(dTime));

    if (startTime <= timeClient.getEpochTime()) {
      Serial.println("---Still in  work, waiting for time...");

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

      if (dTime - 4 <= timeClient.getEpochTime()) {
        digitalWrite(D6, HIGH);
      }

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
