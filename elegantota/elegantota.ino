#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer updater;

const char* serverIndex = 
"<!DOCTYPE html>"
"<html lang='en'>"
"<head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'/>"
    "<title>ElegantOTA</title>"
    "<style>"
        ".upload-btn-wrapper {"
            "position: relative;"
            "overflow: hidden;"
            "display: inline-block;"
        "}"
        ".btn {"
            "border: 2px solid gray;"
            "color: gray;"
            "background-color: white;"
            "padding: 4px 10px;"
            "border-radius: 8px;"
            "font-size: 15px;"
            "font-weight: bold;"
        "}"
        ".upload-btn-wrapper input[type=file] {"
            "font-size: 100px;"
            "position: absolute;"
            "left: 0;"
            "top: 0;"
            "opacity: 0;"
        "}"
        ".loader {"
            "border: 16px solid #f3f3f3;"
            "border-radius: 50%;"
            "border-top: 16px solid #508cfc;"
            "width: 90px;"
            "height: 90px;"
            "-webkit-animation: spin 2s linear infinite;"
            "animation: spin 2s linear infinite;"
        "}"
        "@-webkit-keyframes spin {"
            "0% { -webkit-transform: rotate(0deg); }"
            "100% { -webkit-transform: rotate(360deg); }"
        "}"
        "@keyframes spin {"
            "0% { transform: rotate(0deg); }"
            "100% { transform: rotate(360deg); }"
        "}"
    "</style>"
"</head>"
"<body>"
"<div class='text_center' style='text-align: center'>"
    "<img style='height: 100px; width: 492px;' src='data:image/svg+xml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0idXRmLTgiPz4KPCEtLSBHZW5lcmF0b3I6IEFkb2JlIElsbHVzdHJhdG9yIDIyLjEuMCwgU1ZHIEV4cG9ydCBQbHVnLUluIC4gU1ZHIFZlcnNpb246IDYuMDAgQnVpbGQgMCkgIC0tPgo8c3ZnIHZlcnNpb249IjEuMSIgaWQ9IkxheWVyXzEiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyIgeG1sbnM6eGxpbms9Imh0dHA6Ly93d3cudzMub3JnLzE5OTkveGxpbmsiIHg9IjBweCIgeT0iMHB4IgoJIHZpZXdCb3g9IjAgMCAyNTQgODMiIHN0eWxlPSJlbmFibGUtYmFja2dyb3VuZDpuZXcgMCAwIDI1NCA4MzsiIHhtbDpzcGFjZT0icHJlc2VydmUiPgo8c3R5bGUgdHlwZT0idGV4dC9jc3MiPgoJLnN0MHtmaWxsOiMyRTMwMzQ7fQoJLnN0MXtmaWxsOiM0ODhFRkY7fQo8L3N0eWxlPgo8ZyBpZD0iX3gzMF9iNzQyNDY3LTI3ZWEtYWUzZi03Y2ZhLWNhMGUxMGFjMDRlNyIgdHJhbnNmb3JtPSJtYXRyaXgoMi4yLDAsMCwyLjIsMTAzLjg4NzY0MTY2ODMxOTcsMTMyLjM1NTk5NjAzNjUyOTU1KSI+Cgk8cGF0aCBjbGFzcz0ic3QwIiBkPSJNLTYuMS00NS40di0xSC0xM3Y5LjNoNi44di0xaC01Ljh2LTMuMWg1LjJ2LTFoLTUuMnYtMy4xSC02LjF6IE0tMy42LTQ3aC0xLjF2OS44aDEuMVYtNDd6IE0xLjMtMzgKCQljLTEuNCwwLTIuMy0xLTIuNS0yLjJoNS45di0wLjRjMC0yLTEuMy0zLjYtMy41LTMuNnMtMy41LDEuNi0zLjUsMy42Uy0wLjgtMzcsMS4zLTM3QzMtMzcsNC0zNy45LDQuNS0zOS4ySDMuMwoJCUMzLTM4LjUsMi4zLTM4LDEuMy0zOHogTTEuMi00My4zYzEuMywwLDIuMiwwLjksMi40LDIuMWgtNC44Qy0wLjktNDIuNC0wLjEtNDMuMywxLjItNDMuM3ogTTExLjUtNDQuMnYxLjEKCQljLTAuNS0wLjgtMS4zLTEuMi0yLjUtMS4yYy0yLDAtMy40LDEuNi0zLjQsMy42YzAsMiwxLjMsMy42LDMuNCwzLjZjMS4yLDAsMi0wLjUsMi41LTEuM3YxYzAsMS40LTAuNywyLjItMi4zLDIuMgoJCWMtMS4xLDAtMS44LTAuNC0yLTEuMUg1LjljMC40LDEuMiwxLjUsMiwzLjMsMmMyLjEsMCwzLjMtMS4xLDMuMy0zLjF2LTYuOEgxMS41eiBNOS4xLTM4LjFjLTEuNiwwLTIuNS0xLjItMi41LTIuNgoJCWMwLTEuNCwwLjktMi42LDIuNS0yLjZjMS41LDAsMi40LDEuMiwyLjQsMi42QzExLjUtMzkuMywxMC42LTM4LjEsOS4xLTM4LjF6IE0xNy4zLTQ0LjNjLTIsMC0zLjQsMS42LTMuNCwzLjZzMS4zLDMuNiwzLjQsMy42CgkJYzEuMiwwLDItMC41LDIuNS0xLjJ2MS4xaDF2LTdoLTF2MS4xQzE5LjMtNDMuOCwxOC40LTQ0LjMsMTcuMy00NC4zeiBNMTcuNC0zOGMtMS42LDAtMi41LTEuMi0yLjUtMi43YzAtMS40LDAuOS0yLjcsMi41LTIuNwoJCWMxLjYsMCwyLjQsMS4yLDIuNCwyLjdDMTkuOC0zOS4yLDE5LTM4LDE3LjQtMzh6IE0yMi43LTQ0LjJ2N2gxdi00YzAtMS4zLDEtMi4xLDIuMS0yLjFjMS4yLDAsMiwwLjgsMiwyLjF2NGgxdi00LjEKCQljMC0yLTEuMy0zLTIuOC0zYy0xLjEsMC0xLjgsMC41LTIuMywxLjJ2LTEuMUgyMi43eiBNMzIuMi0zNy4ydi02LjFoMS42di0wLjloLTEuNnYtMi41aC0xLjF2Mi41aC0xLjR2MC45aDEuNHY2LjFIMzIuMnoKCQkgTTM5LjMtNDYuNWMtMi44LDAtNC43LDIuMS00LjcsNC44YzAsMi42LDEuOSw0LjcsNC43LDQuN3M0LjctMi4xLDQuNy00LjdDNDQtNDQuNCw0Mi4xLTQ2LjUsMzkuMy00Ni41eiBNMzkuMy0zOC4xCgkJYy0yLjMsMC0zLjctMS42LTMuNy0zLjdjMC0yLjEsMS40LTMuNywzLjctMy43czMuNywxLjYsMy43LDMuN0M0My0zOS43LDQxLjYtMzguMSwzOS4zLTM4LjF6IE01MS45LTQ2LjRoLTcuNXYxaDMuMnY4LjNoMXYtOC4zCgkJaDMuMlYtNDYuNHogTTU2LjUtNDYuNGgtMS4zbC0zLjksOS4zaDEuMmwxLjEtMi43aDQuNmwxLjEsMi43aDEuMkw1Ni41LTQ2LjR6IE01NC00MC44bDEuOS00LjVsMS45LDQuNUg1NHoiLz4KPC9nPgo8ZyBpZD0iZGM2ODhkMWYtYTA2Ni1iN2U3LThhYmMtMTE1ZmE0MGU5NDQ1IiB0cmFuc2Zvcm09Im1hdHJpeCgwLjIyNTUyMDU3NDg4Mjk3MTExLDAsMCwwLjIyNTUyMDU3NDg4Mjk3MTExLDM0LjUyMjc0Mzk4MTY2Njg1NiwxMTcuMDE3NzQ0OTY3MjczNSkiPgoJPHBhdGggY2xhc3M9InN0MSIgZD0iTTExOS0zNDcuNmM1LjcsMCw3LjgsNi42LDQuNSwxMC44Yy03LjIsOC40LTE0LjQsMTYuNS0yMS42LDI0LjljLTIuMSwyLjQtNi42LDIuNC05LDAKCQljLTcuMi04LjQtMTQuNC0xNi41LTIxLjYtMjQuOWMtNC41LTUuMSwxLjItMTIuMyw2LjMtMTAuOGgxMy4yYy03LjItMjguOC0yOS4xLTUyLjUtNTguOC01OS4xYy01LjctMS4yLTExLjQtMS44LTE3LjEtMS44CgkJYy0yOC44LDAtNTUuNSwxNS45LTY5LjYsNDJjLTMuOSw3LjItMTQuNywwLjktMTAuOC02LjNjMTkuMi0zNC44LDU5LjQtNTQuOSw5OC43LTQ2LjJjMzUuNCw3LjUsNjMsMzYsNzAuMiw3MS40CgkJQzEwMy40LTM0Ny42LDExOS0zNDcuNiwxMTktMzQ3LjZ6IE0tODMuMi0zMDkuMmMtNS43LDAtNy44LTYuNi00LjUtMTAuOGM3LjItOC40LDE0LjQtMTYuNSwyMS42LTI0LjljMi4xLTIuNCw2LjYtMi40LDksMAoJCWM3LjIsOC40LDE0LjQsMTYuNSwyMS42LDI0LjljNC41LDUuMS0xLjIsMTIuMy02LjMsMTAuOEgtNTVjNy4yLDI4LjgsMjkuMSw1Mi41LDU4LjgsNTkuMWM1LjcsMS4yLDExLjQsMS44LDE3LjEsMS44CgkJYzI4LjgsMCw1NS41LTE1LjksNjkuNi00MmMzLjktNy4yLDE0LjctMC45LDEwLjgsNi4zYy0xOS4yLDM1LjEtNTkuNCw1NS4yLTk4LjcsNDYuNWMtMzUuNC03LjgtNjMuMy0zNi4zLTcwLjUtNzEuN0gtODMuMnoiLz4KPC9nPgo8L3N2Zz4K' alt=''>"

    "<br>"
        "<div class='upload-btn-wrapper'>"
            "<button class='btn'>browse</button>"
            "<input type='file' name='update' onchange='uploadOnChange()' id='binfile' accept='.bin,.bin.gz'>"
        "</div>"
    "<br>"
    "<br>"
    "<center><p id='loading' class='loader' hidden='true'></p></center>"
    "<br>"
    "<br>"
    "<label id='lblMsg'></label>"
    "<br>"
    "<br>"
    "<img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAKcAAAAZCAIAAADbtKm+AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAAPwSURBVGhD7Zk/TxtBEMXzvVJGkaJUyReIItGkcYPSUKI0rohAUZrIFQ1yg5SOILmhtERDJAoKKtqAQkhCms27ndnZ2b82hc932E+vsIfx3Xh/O3N35olZa/W0pr6KKlK/v/93e/fn5uevHze33THqQVWojatcqr59Nx++mjdfzMuP5sVOJ4xKUA+qQm0VZahjTbsGOzUqXCJ7rCkWN1rxrhkVltjH1O9+/43Wt8tGtVx3i/o8ide3y0a1qQLq/UJObhl8v5CTU/CeOgZmtKB9cWujHgMzWtC+OBr1nnr3r+Ulo3L+DgtW96/lJaNyLabe30Ynt9Du/W10sm53po7HoWgdxWf7W89fvWUPT/yfJnscHIzP9Fvnjf0Ll3yyHbwVN/HtiXt7Pt7gz+4d+ZzkRDmjfvoiixMeh6J1FE85hXV4YOMH5ooDxlxmMjktzNylyI7ZPOUIJMHGxxw012azHgyN+kVMvTLeQd2DEYNEmYHlx+Tsptkb7W+l1Gk/uYNjB2yNzu1rffD6iZxbGPKV8Q6WHiHZgpQgEBI5yWygOkKH12Z6bDMBzwWnbqM0me61TvDOBhPrIc/Uo0XUPhqqdmRfjAZhO4ZOPwLAMXXsjMF4JJmg6wcJjk87YMaJtOmLLE6Vn2Ly1HMkdKa8xgvpZv2arQ6F/RH/tRBMjfpFc1HnASuDF8CG49GA4/GesDij7kyoM1e/PwLqLl4/UWj6IovTw6hbGGkLZqkHvS5tLZYg8F/aw1rxR7LBnDPU57mBbwYy4QSh7GS2BrDsMNdByfHUm4uCmvB0/OqJtB8y4Sfvn71+yn736YKjM1Wf8FrSeXxtVuyFtJ7wMGNLx4O9UvABgV9x5UNlg5QfOjPhK3dzym7wAobq5hAz2GRmckBdtbWnTnEaKs3kn3miwEu/myutNUzsiZzeH8zScuWPa8bJzoiu37uX5uq0EHRvtTN3c/M9uTmi4QyPyen7fGdFHVuHJ7Y4YTnHiUIv98mtTh2WGZ5mAq3mBGyUGcUbY0+EgJvMbNC91c48uUGFIX+y7Sg2E55fN+QYQDOZfXOXwEQTXpzNVz1dPJF2CzfwpNKQz1A/DphJQiYz7G9OQDC9wOs7APWpbDCyHu+Qp15od7Sda0rVdiqur7UI5i+9c1D3MyDMzJ4ocAuNTiq1O1BpEQO5w4Kk/zLUYXttJnGmipDkU3IufZxsUFs3OuSpQ+v/vszUY/vvC6lf4FtGTuoX+BQ5FFOHMDDneZBbrlFha4M9FQZm5UGuI0aF0WAXZaiTsKZ4HOoaftSDqpbIWwtrischLG7lB5yWjUpQD6oq8SYVqa/1iLWmvopaU189GfMf5c4ucfoN9n4AAAAASUVORK5CYII='>"

    "<script>"
        "function uploadOnChange() {"
            "document.getElementById('binfile').disabled = true;"
            "document.getElementById('loading').hidden = false;"
            
            "var binfile = document.getElementById('binfile');"
            "var formData = new FormData();"
            "formData.append('update', binfile.files[0]);"
            "var xhr = new XMLHttpRequest();"
            
            "xhr.onreadystatechange = function() {"
                "if (this.readyState === 4 && this.status === 200) {"
                "}"
            "};"

            "xhr.upload.onprogress = function(event) {"
                "if (event.lengthComputable) {"
                    "var percentComplete = parseInt((event.loaded / event.total) * 100);"
                    //"console.log('Upload: ' + percentComplete + '% complete');"
                    "document.getElementById('lblMsg').innerHTML='Uploading Code. Please wait...';"
                "}"
            "};"

            "xhr.onloadend = function() {"
                "if (xhr.status === 200) {"
                    "document.getElementById('lblMsg').innerHTML ='Update Success. Rebooting.';"
                    "document.getElementById('loading').hidden = true;"
                    "document.getElementById('binfile').disabled = false;"
                "} else {"
                    "console.log('error '+this.status);"
                    "document.getElementById('lblMsg').innerHTML ='Failed to upload';"
                "}"
            "};"

            "xhr.onerror = function(error) {"
                "console.log('Network Error');"
                "console.log(error);"
            "};"

            "xhr.open('POST', 'update', true);"
            "xhr.send(formData);"
        "}"
    "</script>"
"</div>"
"</body>"
"</html>"
;

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin("Wifi_Er", "wmdcwifier");
  IPAddress ip(192,168,1,192);
  IPAddress gateway(192,168,1,100);
  IPAddress subnet(255,255,255,0);
  IPAddress dns(192,168,1,100);
  WiFi.config(ip, gateway, subnet,dns);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  updater.setup(&server);
  
  server.on("/", []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });  

  server.begin();
  
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
}
