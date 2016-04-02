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
 * Version 1.0 - epierre
 * Converted to 1.4 by Henrik Ekblad
 * 
 * DESCRIPTION
 * Arduino Dust Sensort
 *
 * connect the sensor as follows :
 * 
 *   VCC       >>> 5V
 *   A         >>> A1
 *   GND       >>> GND
 *
 * Based on: http://www.dfrobot.com/wiki/index.php/Sharp_GP2Y1010AU 
 * Authors: Cyrille Médard de Chardon (serialC), Christophe Trefois (Trefex)
 * 
 * http://www.mysensors.org/build/dust
 * 
 * http://www.howmuchsnow.com/arduino/airquality/grovedust/
 * 
 */

// Enable debug prints
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#define MY_NODE_ID 4 //manual node id

#include <SPI.h>
#include <MySensor.h>  

#define CHILD_ID_DUST 0
#define DUST_SENSOR_ANALOG_PIN A1

//VARIABLES
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;
float currentPM10 = 0;

MyMessage dustMsg(CHILD_ID_DUST, V_LEVEL);

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Dust Sensor", "1.1");

  // Register all sensors to gateway (they will be created as child devices)
  present(CHILD_ID_DUST, S_DUST);  
}

void setup() {
  pinMode(DUST_SENSOR_ANALOG_PIN,INPUT);
  starttime = millis();
}

void loop() {    
  // taken from http://www.howmuchsnow.com/arduino/airquality/grovedust/
  // Chris Nafis (c) 2012

 
  duration = pulseIn(DUST_SENSOR_ANALOG_PIN, LOW);
  lowpulseoccupancy = lowpulseoccupancy+duration;

  if ((millis()-starttime) > sampletime_ms)
  {
    ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
    currentPM10 = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve - pcs/283ml

    Serial.print(" - Ratio: ");
    Serial.println(ratio); // unit: pcs/283ml
    Serial.print(" - Dust Density: ");
    Serial.println(currentPM10); // unit: pcs/283ml
    send(dustMsg.set((long)ceil(currentPM10)));
    lowpulseoccupancy = 0;
    starttime = millis();
  }
}
