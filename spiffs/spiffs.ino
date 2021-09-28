#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>

ESP8266WebServer server(80);

void setup() {

  pinMode(2, OUTPUT);
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin("Wifi_Er", "wmdcwifier");
  IPAddress ip(192,168,1,199);
  IPAddress gateway(192,168,1,100);
  IPAddress subnet(255,255,255,0);
  IPAddress dns(192,168,1,100);
  WiFi.config(ip, gateway, subnet,dns);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("");
  Serial.print("Connected to");
  Serial.println("Wifi_Er");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount failed");
  } else {
    Serial.println("SPIFFS Mount successful");
  }

  server.onNotFound([](){
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "404: Not Found");
    }
  });

  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  server.handleClient();
}

bool handleFileRead(String path) {
  Serial.println("handleFileRead: "+path);

  if (path.endsWith("/")) {
    path += "index.html";
  }

  String contentType = getContentType(path);

  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);

    if (path == "/LEDOn.html") {
      digitalWrite(2, LOW);
    } else if (path == "/LEDOff.html") {
      digitalWrite(2, HIGH);  
    }

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
  }

  return "text/plain";
}
