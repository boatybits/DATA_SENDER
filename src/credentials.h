#pragma once // header guard so the file only runs once

#include <Arduino.h>

// Wifi credentials
const char *ssid = "openplotter";
const char *password = "12345678";
const IPAddress mqttServer(10, 10, 10, 1);
const int mqttPort = 1883;
const char *mqttUser = "YOUR USER NAME";
const char *mqttPassword = "YOUR PASSWORD";
const PROGMEM char MQTT_CLIENT_ID[] = "esp32Client";
