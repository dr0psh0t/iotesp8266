#include <FS.h>

String content;

void setup() {
  Serial.begin(115200);
  
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount failed");
  } else {
    Serial.println("SPIFFS Mount successful");
  }

  File file = SPIFFS.open("/configs.txt", "r");

  if (!file) {
    Serial.println("file open failed");
  } else {

    content;

    //  read file
    while (file.available()) {
      //Serial.write(file.read());
      char c = file.read();
      content += c;
    }

    Serial.println("configs.txt content:");
    Serial.println(content);
  }

  file.close();
}

void loop() {
  delay(1500);

  Serial.println(content);
}
