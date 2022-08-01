#include <OneWire.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// https://github.com/milesburton/Arduino-Temperature-Control-Library

//01:15:41.021 -> No more addresses.
//01:15:41.021 ->
//01:15:41.286 -> ROM = 28 A4 25 8C 31 21 3 ED  // adress 1
//01:15:42.314 ->
//01:15:42.314 ->   Temperature = 24.75 Celsius,
//01:15:42.314 -> ROM = 28 AA B0 FF 52 14 1 94  // adress 2
//01:15:43.341 ->
//01:15:43.341 ->   Temperature = 34.44 Celsius,
//01:15:43.341 -> ROM = 28 AA 1F DD 52 14 1 BF
//01:15:44.368 ->
//01:15:44.368 ->   Temperature = 24.56 Celsius,
//01:15:44.368 -> No more addresses.


OneWire  ds(13);  // on pin 10 (a 4.7K resistor is necessary)

float BatteryV = 4.2;     // asuming useing a LiPo battery of some kind.
float BatteryLevle = 100; // asuming its in %

byte i;
byte present = 0;
byte type_s;
byte data[9];
 byte addr[8];
float celsius, fahrenheit;

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

void GetTemp( byte addr[]  )
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

  GetBat();
}

void loop(void) {



  for (int i = 0; i <= 2; i++) {

    switch (i)
    {
      case 0:
        { 
            Serial.println("sensor1");
            GetTemp(addr1);
        }break;

              case 1:
        { 
            Serial.println("sensor2");
            GetTemp(addr2);
        }break;

              case 2:
        { 
            Serial.println("sensor3");
            GetTemp(addr3);
        }break;

       DEFUALT:
        {
          Serial.println("whoops!");
        }
    }




  }








}
