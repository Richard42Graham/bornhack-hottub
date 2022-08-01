#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// https://github.com/milesburton/Arduino-Temperature-Control-Library

// 01:15:41.021 -> No more addresses.
// 01:15:41.021 ->
// 01:15:41.286 -> ROM = 28 A4 25 8C 31 21 3 ED  // adress 1
// 01:15:42.314 ->
// 01:15:42.314 ->   Temperature = 24.75 Celsius,
// 01:15:42.314 -> ROM = 28 AA B0 FF 52 14 1 94  // adress 2
// 01:15:43.341 ->
// 01:15:43.341 ->   Temperature = 34.44 Celsius,
// 01:15:43.341 -> ROM = 28 AA 1F DD 52 14 1 BF
// 01:15:44.368 ->
// 01:15:44.368 ->   Temperature = 24.56 Celsius,
// 01:15:44.368 -> No more addresses.

OneWire wire(13);            // on pin 10 (a 4.7K resistor is necessary)
float battery_voltage = 4.2; // asuming useing a LiPo battery of some kind.
float battery_level = 100;   // asuming its in %
byte i;
byte present = 0;
byte type_s;
byte data[9];
byte addr1[8] = {0x28, 0xA4, 0x25, 0x8C, 0x31, 0x21, 0x03, 0xED}; // sensor 1
byte addr2[8] = {0x28, 0xAA, 0xB0, 0xFF, 0x52, 0x14, 0x01, 0x94}; // sensor 2
byte addr3[8] = {0x28, 0xAA, 0x1F, 0xDD, 0x52, 0x14, 0x01, 0xBF}; // sensor 3

float latest_temperature_1 = 0;
float latest_temperature_2 = 0;
float latest_temperature_3 = 0;

unsigned long pool_frequency_ms = 5000;
int temperature_acquisition_time_ms = 1000;
unsigned long last_poll_time = 0;
unsigned long temperature_start_read_time = 0;
bool have_read_temperature = false;

WiFiClient espClient;
ESP8266WebServer httpServer(80);


void handleRoot() {
  httpServer.send(200, "text/html","Hey you found me dashboard @ https://richard42graham.github.io/bornhack-hottub or /data");
}

void handle_temperature_1() {
  String output ="[";
  output.concat(latest_temperature_1);
  //for (int i = 0; i < 1000; i++) {
  //  output.concat(records[i]);
  //  if(i != 999){
  //    output.concat(",");
  //  }
  //}
  output.concat("]");
  httpServer.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  httpServer.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.send(200, "application/json",output);
}

void handle_temperature_2() {
  String output ="[";
  output.concat(latest_temperature_2);
  //for (int i = 0; i < 1000; i++) {
  //  output.concat(records[i]);
  //  if(i != 999){
  //    output.concat(",");
  //  }
  //}
  output.concat("]");
  httpServer.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  httpServer.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.send(200, "application/json", output);
}

void handle_temperature_3() {
  String output ="[";
  output.concat(latest_temperature_2);
  //for (int i = 0; i < 1000; i++) {
  //  output.concat(records[i]);
  //  if(i != 999){
  //    output.concat(",");
  //  }
  //}
  output.concat("]");
  httpServer.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  httpServer.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.send(200, "application/json", output);
}

void handle_battery() {
  String output ="{";
  output.concat("\"level\":");
  output.concat(battery_level);
  output.concat(",\"voltage\":");
  output.concat(battery_voltage);
  output.concat("}");
  httpServer.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  httpServer.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.send(200, "application/json", output);
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


void ReadBattery()
{
  // sensorValue * (5.0 / 1023.0);
  battery_voltage = analogRead(A0) * (4.5 / 1024);
  battery_level = map(battery_voltage, 3.3, 4.3, 5, 100); // calculate battery "level by mapping the voltage"
  //  map(long x, long in_min, long in_max, long out_min, long out_max)

  Serial.print("battery voltage = "); // show battery voltage
  Serial.print(battery_voltage);
  Serial.println(" ");

  Serial.print("battery level = "); // show battery levle
  Serial.print(battery_level);
  Serial.println("% ");
}

void StartMeasuringTemperature(byte address[])
{
  if (OneWire::crc8(address, 7) != address[7])
  {
    Serial.println("CRC is not valid!");
    return;
  }
  wire.reset();
  wire.select(address);
  wire.write(0x44, 1); // start conversion, with parasite power on at the end
}

float ReadTemperature(byte address[])
{
  // we might do a ds.depower() here, but the reset will take care of it.
  present = wire.reset();
  wire.select(address);
  wire.write(0xBE); // Read Scratchpad
  for (i = 0; i < 9; i++)
  { // we need 9 bytes
    data[i] = wire.read();
  }
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s)
  {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10)
    {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  }
  else
  {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00)
      raw = raw & ~7; // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20)
      raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40)
      raw = raw & ~1; // 11 bit res, 375 ms
  /// default is 12 bit resolution, 750 ms conversion time
  }
  float celsius = (float)raw / 16.0;
  // fahrenheit = celsius * 1.8 + 32.0;
  return celsius;
}

void setup(void)
{
  Serial.begin(115200);
  pinMode(A0, INPUT);
  ReadBattery();

  // Wifi
  delay(300);
  // Setup wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin("SSID", "Password");
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

  // HTTP Server
  httpServer.on("/data", HTTP_OPTIONS, []() {
    httpServer.sendHeader("Access-Control-Max-Age", "10000");
    httpServer.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    httpServer.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.send(200, "text/plain", "" );
  });
  httpServer.on("/", handleRoot);
  httpServer.on("/data/temperature_1", handle_temperature_1);
  httpServer.on("/data/temperature_2", handle_temperature_2);
  httpServer.on("/data/temperature_3", handle_temperature_3);
  httpServer.on("/data/battery", handle_battery);
  httpServer.onNotFound(handleNotFound);
  httpServer.begin();
}

void loop(void)
{

  if (last_poll_time + pool_frequency_ms < millis())
  {
    // Starts a new temperature measurement
    last_poll_time = millis();
    StartMeasuringTemperature(addr1);
    StartMeasuringTemperature(addr2);
    StartMeasuringTemperature(addr3);
    temperature_start_read_time = millis();
    have_read_temperature = false;
    Serial.println("Starts reading temperature: ");
    
  }

  if (!have_read_temperature && temperature_start_read_time + temperature_acquisition_time_ms < millis())
  {
    // Reads the temperature
    float temperature1 = ReadTemperature(addr1);
    float temperature2 = ReadTemperature(addr2);
    float temperature3 = ReadTemperature(addr3);

    Serial.print("temperature1: ");
    Serial.print(temperature1);
    Serial.print(", temperature2: ");
    Serial.print(temperature2);
    Serial.print(", temperature3: ");
    Serial.print(temperature3);
    Serial.println(" ");
    latest_temperature_1 = temperature1;
    latest_temperature_2 = temperature2;
    latest_temperature_3 = temperature3;
    have_read_temperature = true;
  }
}
