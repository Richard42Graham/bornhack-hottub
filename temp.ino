#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "http.h"
#define Addr 0x48
const char* mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;
float cTemp;
float records[1000];
WiFiClient espClient;
PubSubClient mqttClient(espClient);
ESP8266WebServer httpServer(80);

void handleRoot() {
  httpServer.send(200, "text/html","Hey you found me dashboard @ https://richard42graham.github.io/bornhack-hottub or /data");
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
    httpServer.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    httpServer.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.send(200, "application/json",output);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += httpServer.uri();
  message += "\nMethod: ";
  message += (httpServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += httpServer.args();
  message += "\n";
  for (uint8_t i = 0; i < httpServer.args(); i++) {
    message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
  }
  httpServer.send(404, "text/plain", message);
}


bool mqttConnect(){
  mqttClient.setServer(mqttServer, mqttPort);
  return mqttClient.connect("ESP8266Client", "/dk/bornhack/hottub/status", 0, true, "0");
}


void setup() {
  Serial.begin(115200);
  Serial.println("We're starting up");
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

  
  while (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT...");
    if (mqttConnect()) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      delay(2000);
      }
    }


  httpServer.on("/data", HTTP_OPTIONS, []() {
    httpServer.sendHeader("Access-Control-Max-Age", "10000");
    httpServer.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    httpServer.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.send(200, "text/plain", "" );
  });
  httpServer.on("/", handleRoot);
  httpServer.on("/data", handleData);

  httpServer.onNotFound(handleNotFound);
  httpServer.begin();
}



int lastUpdate = 0;
void loop() {
  if (!mqttClient.connected()) {
    mqttConnect();
  }
  mqttClient.loop();
  if (lastUpdate + 20000 < millis()) {
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
    memcpy(records, &records[1], sizeof(records) - sizeof(float));
    if(cTemp == 16767440){
      cTemp = 0;
    }
    records[999] =  cTemp;
    // Output data to serial monitor
    Serial.print("Temperature in Celsius:  ");
    Serial.print(cTemp);
    Serial.println(" C");

    // check if we're connected, try to reconnect once
    if(!mqttClient.connected()) {
      Serial.println("disconnected to mqtt, try to reconnect");
      mqttConnect();
    }

    // check again, simply give up publishing if we're still not connected
    // we'll retry after reading the next value anyhow
    if(mqttClient.connected()){
      Serial.println("connected to mqtt, publishing");
      char buf[10];
      sprintf(buf, "%.2f", cTemp);
      mqttClient.publish("/dk/bornhack/hottub/water_temp", buf, true);
      mqttClient.publish("/dk/bornhack/hottub/status", "1");
    }
  }
  httpServer.handleClient();
}
