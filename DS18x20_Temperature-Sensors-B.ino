#include <OneWire.h>

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
byte addr[8];
byte addr1[8] = {0x28, 0xA4, 0x25, 0x8C, 0x31, 0x21, 0x03, 0xED}; // sensor 1
byte addr2[8] = {0x28, 0xAA, 0xB0, 0xFF, 0x52, 0x14, 0x01, 0x94}; // sensor 2
byte addr3[8] = {0x28, 0xAA, 0x1F, 0xDD, 0x52, 0x14, 0x01, 0xBF}; // sensor 3

int pool_frequency_ms = 20000;
int temperature_acquisition_time_ms = 1000;
int last_poll_time = 0;
int temperature_start_read_time = 0;
bool have_read_temperature = false;

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
  Serial.println();
  wire.reset();
  wire.select(address);
  wire.write(0x44, 1); // start conversion, with parasite power on at the end
}

float ReadTemperature(byte address[])
{
  // we might do a ds.depower() here, but the reset will take care of it.
  present = wire.reset();
  wire.select(addr);
  wire.write(0xBE); // Read Scratchpad
  for (i = 0; i < 9; i++)
  { // we need 9 bytes
    data[i] = wire.read();
  }
  Serial.println();
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
    //// default is 12 bit resolution, 750 ms conversion time
  }
  float celsius = (float)raw / 16.0;
  // fahrenheit = celsius * 1.8 + 32.0;
  // Serial.print("Temperature = ");
  // Serial.print(celsius);
  // Serial.print(" Celsius, ");
  // Serial.println(" ");
  return celsius;
}

void setup(void)
{
  Serial.begin(115200);
  pinMode(A0, INPUT);
  ReadBattery();
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
  }
}
