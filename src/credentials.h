#pragma once // header guard so the file only runs once

#include <Arduino.h>

// Wifi credentials//
const char *ssid = "openplotter";
const char *password = "12345678";
const IPAddress mqttServer(10, 10, 10, 1);
const int mqttPort = 1883;
const char *mqttUser = "cafonhvu";
const char *mqttPassword = "Usp_VhNLl897";
const PROGMEM char MQTT_CLIENT_ID[] = "esp32Client";
