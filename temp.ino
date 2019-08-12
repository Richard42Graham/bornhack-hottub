#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "http.h"
#define Addr 0x48
int cTemp;
int records[1000];
ESP8266WebServer server(80);

void handleRoot() {
  server.send(200, "text/html","Hey you found me dashboard @ https://richard42graham.github.io/bornhack-hottub or /data");
}
void handleData() {
  String output ="[";
    for (int i = 0; i < 1000; i++) {
      output.concat(records[i]);
      if(i != 999){
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
    records[999] =  cTemp;
    // Output data to serial monitor
    Serial.print("Temperature in Celsius:  ");
    Serial.print(cTemp);
    Serial.println(" C");
  }
  server.handleClient();

}
