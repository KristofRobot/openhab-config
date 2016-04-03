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
 * Version 1.0 - Henrik EKblad
 * Version 2.0 - Added Battery sensing (Kristof Robot)
 * 
 * DESCRIPTION
 * This sketch provides an example how to implement a humidity/temperature
 * sensor using DHT11/DHT-22 
 * http://www.mysensors.org/build/humidity
 */
 
// Enable debug prints
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#define MY_NODE_ID 5 //manual node id
//#define MY_DISABLED_SERIAL //disable serial

#include <SPI.h>
#include <MySensor.h>  
#include <DHT.h>  

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define HUMIDITY_SENSOR_DIGITAL_PIN 3
unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int oldBatteryPcnt = 0;
int SAMPLES=100; //how many samples to take for voltage reading
const long InternalReferenceVoltage = 1090L;  // Detected using the method described at http://arduino.cc/forum/index.php/topic,38119.0.html
const int MIN_BATTERY = 700; //2 li Ion batteries, minimum is 7V
const int MAX_BATTERY = 840; //2 li Ion batteries, max is 8.4V

DHT dht;
float lastTemp;
float lastHum;
boolean metric = true; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);


void setup()  
{ 
  analogReference(INTERNAL);
  
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 

  metric = getConfig().isMetric;
}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("HumidityBattery", "1.1");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
}


void loop()      
{  
  //get battery voltage - battery connected through even voltage divider of 2*10K
  //oversampling (seems overkill, but doing it anyway)
  long Vcc=0L;
  long currentVoltage=0L;
  for (int i=0; i < SAMPLES; i++){
        Vcc += readVcc();
        currentVoltage += analogRead(BATTERY_SENSE_PIN); 
        delay(10); 
  }
  Vcc=Vcc/SAMPLES; //get average from SAMPLES readings; dont care about truncation
  currentVoltage = map(currentVoltage/SAMPLES, 0, 1023, 0, Vcc);
  Serial.println(Vcc);
  Serial.println(currentVoltage);
  
  int batteryPcnt = map(currentVoltage*2,MIN_BATTERY, MAX_BATTERY, 0, 100); //convert from voltage to percentage; using even divider, so need to multiply by 2
  Serial.println(batteryPcnt);
  if (oldBatteryPcnt != batteryPcnt) {
     sendBatteryLevel(batteryPcnt);
     oldBatteryPcnt = batteryPcnt;
   }
  
  delay(dht.getMinimumSamplingPeriod());

  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
  } else if (temperature != lastTemp) {
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    send(msgTemp.set(temperature, 1));
    Serial.print("T: ");
    Serial.println(temperature);
  }
  
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum) {
      lastHum = humidity;
      send(msgHum.set(humidity, 1));
      Serial.print("H: ");
      Serial.println(humidity);
  }

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


