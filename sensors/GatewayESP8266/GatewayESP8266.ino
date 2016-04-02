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
 * Contribution by a-lurker and Anticimex, 
 * Contribution by Norbert Truchsess <norbert.truchsess@t-online.de>
 * Contribution by Ivo Pullens (ESP8266 support)
 * 
 * DESCRIPTION
 * The EthernetGateway sends data received from sensors to the WiFi link. 
 * The gateway also accepts input on ethernet interface, which is then sent out to the radio network.
 *
 * VERA CONFIGURATION:
 * Enter "ip-number:port" in the ip-field of the Arduino GW device. This will temporarily override any serial configuration for the Vera plugin. 
 * E.g. If you want to use the defualt values in this sketch enter: 192.168.178.66:5003
 *
 * LED purposes:
 * - To use the feature, uncomment WITH_LEDS_BLINKING in MyConfig.h
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error  
 * 
 * See http://www.mysensors.org/build/esp8266_gateway for wiring instructions.
 * nRF24L01+  ESP8266
 * VCC        VCC
 * CE         GPIO4          
 * CSN/CS     GPIO15
 * SCK        GPIO14
 * MISO       GPIO12
 * MOSI       GPIO13
 * GND        GND
 *            
 * Not all ESP8266 modules have all pins available on their external interface.
 * This code has been tested on an ESP-12 module.
 * The ESP8266 requires a certain pin configuration to download code, and another one to run code:
 * - Connect REST (reset) via 10K pullup resistor to VCC, and via switch to GND ('reset switch')
 * - Connect GPIO15 via 10K pulldown resistor to GND
 * - Connect CH_PD via 10K resistor to VCC
 * - Connect GPIO2 via 10K resistor to VCC
 * - Connect GPIO0 via 10K resistor to VCC, and via switch to GND ('bootload switch')
 * 
  * Inclusion mode button:
 * - Connect GPIO5 via switch to GND ('inclusion switch')
 * 
 * Hardware SHA204 signing is currently not supported!
 *
 * Make sure to fill in your ssid and WiFi password below for ssid & pass.
 */

#include <EEPROM.h>
#include <SPI.h>

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Gateway mode always enabled for ESP8266. But we add this anyway ;)
#define MY_GATEWAY_ESP8266

// Enable UDP communication
//#define MY_USE_UDP

// The port to keep open on node server mode 
#define MY_PORT 5003      

// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 1

// Controller ip address. Enables client mode (default is "server" mode). 
// Also enable this if MY_USE_UDP is used and you want sensor data sent somewhere. 
//#define MY_CONTROLLER_IP_ADDRESS 192, 168, 178, 68

// Enable inclusion mode
//#define MY_INCLUSION_MODE_FEATURE

// Enable Inclusion mode button on gateway
// #define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
//#define MY_INCLUSION_MODE_DURATION 60 
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN  3 

 
// Flash leds on rx/tx/err
// #define MY_LEDS_BLINKING_FEATURE
// Set blinking period
// #define MY_DEFAULT_LED_BLINK_PERIOD 300

// Led pins used if blinking feature is enabled above
//#define MY_DEFAULT_ERR_LED_PIN 16  // Error led pin
//#define MY_DEFAULT_RX_LED_PIN  16  // Receive led pin
//#define MY_DEFAULT_TX_LED_PIN  16  // the PCB, on board LED

#define MY_ESP8266_SSID ""
#define MY_ESP8266_PASSWORD ""

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
#define MY_IP_ADDRESS 192,168,0,201

// If using static ip you need to define Gateway and Subnet address as well
#define MY_IP_GATEWAY_ADDRESS 192,168,0,1
#define MY_IP_SUBNET_ADDRESS 255,255,255,0

#if defined(MY_USE_UDP)
  #include <WiFiUDP.h>
#else
  #include <ESP8266WiFi.h>
#endif

#include <MySensor.h>

/*** variables for locally attached humidity sensor **/
#include <DHT.h>  
#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define HUMIDITY_SENSOR_DIGITAL_PIN 0    //D0 according to https://github.com/esp8266/Arduino/blob/master/doc/reference.md - this is labeled D3 on my ESP! Do continuity test!

DHT dht;
float lastTemp;
float lastHum;
boolean metric = true; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
unsigned long starttime;
unsigned long sampletime_ms = 30000;

void setup() {
    dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 

    metric = getConfig().isMetric; 
    starttime = millis();
    Serial.println("End of DHT setup");
}

void presentation() {
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Humidity", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);   
}


void loop() {

  if ((millis()-starttime) > sampletime_ms){
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
    starttime = millis();
  }
}



