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
 * Version 0.2 - Turn of hall effect sensor in between measurements to save power
 * Version 0.1 - Based on humidity sensor by Henrik EKblad
 * 
 * DESCRIPTION
 * This sketch implements hall effect sensor monitoring
 * based on http://www.mysensors.org/build/humidity
 */
 
// Enable debug prints
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#define MY_NODE_ID 6 //manual node id
//#define MY_DISABLED_SERIAL //disable serial

#include <SPI.h>
#include <MySensor.h>  
#include <DHT.h>  

#define CHILD_ID_HALL 0
int HALL_SENSOR_POWER_PIN = 4;
int HALL_SENSOR_ANALOG_PIN = A5;
unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

//int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int SAMPLES=100; //how many samples to take for voltage reading
const long InternalReferenceVoltage = 1080L;  // Detected using the method described at http://arduino.cc/forum/index.php/topic,38119.0.html
//3 - 1080
//5 - 1095
//const int MIN_BATTERY = 700; //2 li Ion batteries, minimum is 7V
//const int MAX_BATTERY = 840; //2 li Ion batteries, max is 8.4V

//NiMH - note: discharge curve of NiMH is almost flat at 1.2V for most of the time - i.e. voltage measurement is not a very good indicator
//const int MIN_BATTERY = 660; //6 NiMH cell pack, minimum is 6.6V (in theory 0.9V per cell, i.e. 5.4V, but since 6 in series, be conservative)
//const int MAX_BATTERY = 840; //6 NiMH cell pack, max is 8.4V  

//NiMH - note: discharge curve of NiMH is almost flat at 1.2V for most of the time - i.e. voltage measurement is not a very good indicator
const int MIN_BATTERY = 480; //4 NiMH eneloop cell pack, minimum is 1.0V (at low power, 1.2V is near minimum) - 
const int MAX_BATTERY = 560; //4 NiMH eneloop cell pack, max is 1.4, quickly going to 1.3

//abusing temp for now
MyMessage msgHall(CHILD_ID_HALL, V_TEMP);


void setup()  
{ 
  //Serial.begin(115200);
  pinMode(HALL_SENSOR_POWER_PIN, OUTPUT);
  //disable power
  digitalWrite(HALL_SENSOR_POWER_PIN, LOW);
}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("HallEffectBattery", "0.1");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HALL, S_TEMP);
}


void loop()      
{  
  //enable sensor
  digitalWrite(HALL_SENSOR_POWER_PIN, HIGH);
  sleep(100);

  //get battery voltage - battery connected through even voltage divider of 2*10K
  //oversampling (seems overkill, but doing it anyway)
  long Vcc=0L;
  long hallSensorValue=0L;
  for (int i=0; i < SAMPLES; i++){
        Vcc += readVcc();
        hallSensorValue += analogRead(HALL_SENSOR_ANALOG_PIN); 
        delay(10); 
  }
  //disable sensor
  digitalWrite(HALL_SENSOR_POWER_PIN, LOW);

  Vcc=Vcc/SAMPLES; //get average from SAMPLES readings; dont care about truncation
  //Serial.println(Vcc);
  
  int batteryPcnt = constrain(map(Vcc,MIN_BATTERY, MAX_BATTERY, 0, 100),0,100); //convert from voltage to percentage; constrain to 0-100 range
  //Serial.println(batteryPcnt);
  //always send batteryPcnt; use this as heartbeat
  sendBatteryLevel(batteryPcnt);
  
  hallSensorValue=hallSensorValue/SAMPLES;
  send(msgHall.set(hallSensorValue, 1));
  //Serial.println(hallSensorValue);
  
  sleep(SLEEP_TIME); //power down, defined in MySensorCore.cpp - akin to lowpower.h power down routine
}

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
  
  
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  long result = (((InternalReferenceVoltage*1023L) / ADC)+5L)/10L; 
  return result; // Vcc in millivolts
}


