/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 0.1 - Based on forum code at https://forum.mysensors.org/topic/7436/bme-280-how-to-use-it/6
 * 
 * DESCRIPTION
 * This sketch implements BME280 sensor monitoring
 */
 
// Enable debug prints
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#define MY_NODE_ID 7 //manual node id
//Serial not supported when bme280 connected
#define MY_DISABLED_SERIAL //disable serial

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <MySensor.h>  

#define SEALEVELPRESSURE_HPA (1023.0)
#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_BARO 2

Adafruit_BME280 bme; // I2C
const uint8_t BME280_i2caddr = 0x76;
bool metric = true;
unsigned long SLEEP_TIME = 60000;

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgPress(CHILD_ID_BARO, V_PRESSURE);

void setup()
{
  metric = getConfig().isMetric; 
  Wire.begin();
  if (!bme.begin(BME280_i2caddr)) {
    while (1);
  }
  // weather monitoring - to avoid overheating
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
    Adafruit_BME280::SAMPLING_X1, // temperature
    Adafruit_BME280::SAMPLING_X1, // pressure
    Adafruit_BME280::SAMPLING_X1, // humidity
    Adafruit_BME280::FILTER_OFF);
  // suggested rate is 1/60Hz (1m)
  
  SLEEP_TIME = 60000;
  
}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("BME280Sensor", "0.1");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
  present(CHILD_ID_BARO, S_BARO);
}


void loop()
{
  bme.takeForcedMeasurement();
  float HUM = bme.readHumidity();
  float TEMP = bme.readTemperature();
  float BARO = bme.readPressure() / 100;
  send(msgHum.set(HUM, 2));
  send(msgTemp.set(TEMP, 2));
  send(msgPress.set(BARO, 1));

  sleep(SLEEP_TIME); //power down, defined in MySensorCore.cpp - akin to lowpower.h power down routine

}
