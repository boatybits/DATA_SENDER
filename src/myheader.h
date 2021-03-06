#pragma once // header guard so the file only runs once

#include <Arduino.h>
#include "credentials.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "PubSubClient.h" // MQTT
#include "SparkFunBME280.h"
#include "ArduinoJson.h"
#include "Adafruit_INA219.h" //https://github.com/adafruit/Adafruit_INA219
#include "Adafruit_ADS1015.h"
#include "LiquidCrystal_I2C.h"

// call functions so they can be put under main loop and compiler still sees them first
void LogOn();
void callback(char *intopic, byte *payload, unsigned int length);
void sendMQTT(String topic, String message);
void sendSigK(String sigKey, float data);
void send_Data(int send_Data_Rate);
void handle_OnConnect();
void handle_led1on();
void handle_led1off();
void handle_NotFound();
String SendHTML(uint8_t led1stat);

const IPAddress remoteIp(10, 10, 10, 1); //
const unsigned short remotePort = 55561; // Signalk listens on this port
const unsigned short localPort = 6666;   // local port to listen for UDP packets

uint8_t LED1pin = 2;
bool LED1status = LOW;

// Timers for loops
int send_Data_Rate = 1000; // loop time in mS for a2_sendData_Rate
int check_online_Rate = 100;
int counter = 0;

unsigned long timer1_Millis; // send data
unsigned long timer2_Millis = 10000;
unsigned long timer3_Millis = 1000;
unsigned long timer4_Millis = 1000;
unsigned long timer5_Millis = 1000;
unsigned long timer6_Millis = 1000;
int rebooted = 0;
String rebootedString = "0";

// Flags
int quickprint_flag = 1;
int sendSig_Flag = 1;
int sendInfdb_Flag = 1;
unsigned long currentMillis;

//                         Start instances of libraries
WiFiClient espClient;
WebServer server(80);
PubSubClient client(espClient); // MQTT
BME280 BE280;                   //BME280 barometer
WiFiUDP Udp;                    // A UDP instance to let us send and receive packets over UDP
Adafruit_INA219 ina219;
Adafruit_ADS1115 ads(0x4A);
Adafruit_ADS1115 ads2(0x4A);
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display