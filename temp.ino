#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "http.h"
#define Addr 0x48
int cTemp;
int records[100];
ESP8266WebServer server(80);

String c1 = "<html>\r\n<head><style>.bar{background-color:#000;color:#fff;display:inline-block;width:1%;position:fixed;bottom:0}.barHold{width:1%;display:inline-block}#bars{height:100%;width:100%}body,html{height:100%;width:100%;margin:0;padding:0}</style></head><body>\r\n<div id=\"bars\">\r\n</div><script>setInterval(async function (){\r\n  let resp = await fetch(\"/data\")\r\n  let values = await resp.json();\r\nlet min = 0;\r\nlet max = values.reduce((acu, val) => (val > acu ? val : acu), 0);\r\nlet el = document.getElementById(\"bars\");\r\n  el.innerHTML = \"\";\r\nfor (let val of values) {\r\n  let bar = document.createElement(\"div\");\r\n  bar.classList += \"bar\";\r\n  bar.innerText = val;\r\n  bar.style.height = (val - min) / (max - min) * 100 + \"%\";\r\n  let barHold = document.createElement(\"div\");\r\n  barHold.classList += \"barHold\";\r\n  barHold.appendChild(bar);\r\n  el.appendChild(barHold);\r\n}\r\n},1000)\r\n</script>\r\n</head>\r\n</body>\r\n</html>";
void handleRoot() {
  server.send(200, "text/html",c1);
}
void handleData() {
  String output ="[";
    for (int i = 0; i < 100; i++) {
      output.concat(records[i]);
      if(i != 99){
        output.concat(",");
      }
    }
    output.concat("]");
        server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
                server.sendHeader("Access-Control-Allow-Origin", "*");



  server.send(200, "application/json",output);
}
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
    Serial.begin(115200);
    
  // put your setup code here, to run once:
  pinMode(5, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  Wire.begin(5, 4); // join i2c bus (address optional for master)
  // put your main code here, to run repeatedly:
  Wire.beginTransmission(Addr);
  Wire.write(0x01);
  Wire.write(0x60);
  Wire.write(0xA0);
  Wire.endTransmission();
  
  delay(300);
  // Setup wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin("bornhack");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (MDNS.begin("the-tub")) {
    Serial.println("MDNS responder started");
  }
server.on("/data", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Max-Age", "10000");
    server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
        server.sendHeader("Access-Control-Allow-Origin", "*");

    server.send(200, "text/plain", "" );
  });
  server.on("/", handleRoot);
    server.on("/data", handleData);

  server.onNotFound(handleNotFound);
  server.begin();
}
int lastUpdate = 0;
void loop() {
  if (lastUpdate + 1000 < millis()) {
    lastUpdate = millis();
    unsigned data[2];
    // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Select data register
    Wire.write(0x00);
    // Stop I2C Transmission
    Wire.endTransmission();
    delay(300);
    // Request 2 bytes of data
    Wire.requestFrom(Addr, 2);
    // Read 2 bytes of data
    // temp msb, temp lsb
    if (Wire.available() == 2)
    {
      data[0] = Wire.read();
      data[1] = Wire.read();
    }
    // Convert the data to 12-bits
    int temp = ((data[0] * 256) + data[1]) / 16;
    if (temp > 2048)
    {
      temp -= 4096;
    }
    cTemp = temp * 0.0625;
    memcpy(records, &records[1], sizeof(records) - sizeof(int));
    if(cTemp == 16767440){
      cTemp = 0;
    }
    records[99] =  cTemp;
    // Output data to serial monitor
    Serial.print("Temperature in Celsius:  ");
    Serial.print(cTemp);
    Serial.println(" C");
  }
  server.handleClient();

}
