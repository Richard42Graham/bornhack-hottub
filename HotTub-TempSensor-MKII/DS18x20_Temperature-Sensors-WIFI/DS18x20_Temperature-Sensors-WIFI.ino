
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "http.h"

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


int lastUpdate = 0;
























#include <OneWire.h>
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// https://github.com/milesburton/Arduino-Temperature-Control-Library

OneWire  ds(13);  // on pin 10 (a 4.7K resistor is necessary)

float BatteryV = 4.2;     // asuming useing a LiPo battery of some kind.
float BatteryLevle = 100; // asuming its in %

byte i;
byte present = 0;
byte type_s;
byte data[9];
 byte addr[8];
float celsius;

byte addr1[8] = {0x28, 0xA4, 0x25, 0x8C, 0x31, 0x21, 0x03, 0xED}; // sensor 1
byte addr2[8] = {0x28, 0xAA, 0xB0, 0xFF, 0x52, 0x14, 0x01, 0x94}; // sensor 2
byte addr3[8] = {0x28, 0xAA, 0x1F, 0xDD, 0x52, 0x14, 0x01, 0xBF}; // sensor 3


void GetBat()
{
  // sensorValue * (5.0 / 1023.0);
  BatteryV = analogRead(A0) * ( 4.5 / 1024);
  BatteryLevle = map(BatteryV, 3.3, 4.3,  5,  100); // calculate battery "level by mapping the voltage"
  //  map(long x, long in_min, long in_max, long out_min, long out_max)

  Serial.print("battery voltage = "); // show battery voltage
  Serial.print(BatteryV);
  Serial.println(" ");

  Serial.print("battery levle = "); // show battery levle
  Serial.print(BatteryLevle);
  Serial.println("% ");
}

void GetTemp( byte addr[]  )  // should this return a tmep ? return tempC ? 
{

    if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return;
  }
  Serial.println();
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
  Serial.println();
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.println(" ");
}


void setup(void) {
  Serial.begin(115200);
  //Serial.println("hello world");  // debug
  pinMode(A0, INPUT);

  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }

  Serial.println("We're starting up");
  // put your setup code here, to run once:
 
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
//  GetBat();
}




void loop(void) {



 if (!mqttClient.connected()) {
    mqttConnect();
  }
  mqttClient.loop();
  if (lastUpdate + 20000 < millis()) {
    lastUpdate = millis();

      // poll sensors here ? 
      // GetTemp(addr1);
      // GetTemp(addr2);
      // GetTemp(addr3);
      // GetBat();
      //
      // put temps in cTemp i think

    }


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
