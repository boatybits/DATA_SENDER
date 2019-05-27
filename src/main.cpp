#include <Arduino.h>
#include "myheader.h"

//**********************************************************************************************
//                         Run Setup
//**********************************************************************************************
//
void setup()
{
  Serial.begin(115200);
  pinMode(LED1pin, OUTPUT);
  //Wire.begin(5, 4); // pins 5 & 4 for wemos
  Wire.begin(21, 22); //

  LogOn();
  BE280.setI2CAddress(0x76); //Connect to BME on ox76
  if (BE280.beginI2C() == false)
    Serial.println("BE280 connect failed");
  else
    Serial.println("BE280 connected");
  delay(250); // let the BME settle down

  ina219.begin();
  ina219.setCalibration_32V_2A();
  //ina219.setCalibration_16V_400mA();

  ads2.setGain(GAIN_TWOTHIRDS);
  ads2.begin();

  server.on("/", handle_OnConnect);
  server.on("/led1on", handle_led1on);
  server.on("/led1off", handle_led1off);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
  //LCD
  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.setCursor(0, 1);
  lcd.print("Booted. Hi World");
}
//________________________________________________________________________________

//********************************************************************************
//                         Run Main Loop
//********************************************************************************
void loop(void)
{

  client.loop();
  currentMillis = millis();
  send_Data(1000);

  server.handleClient();
  if (LED1status)
  {
    digitalWrite(LED1pin, HIGH);
  }
  else
  {
    digitalWrite(LED1pin, LOW);
  }
}
//________________________________________________________________________________

//********************************************************************************
//********************************************************************************
//                         SUBROUTINES
//*******************************************************************************
//*******************************************************************************

//                     Log o to wifi network
//*******************************************************
void LogOn()
{
  Serial.println("Booting");
  WiFi.disconnect(true); // disconnects STA Mode
  delay(1000);
  WiFi.softAPdisconnect(true); // disconnects AP Mode
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(1500);
    ESP.restart();
  }
  // Log on successful
  Serial.println("Logged on");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Now setting up MQTT");

  //________________MQTT stuff____________________________________________
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  while (!client.connected())
  {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword))
    {
      Serial.println("connected");
      delay(50);
      sendMQTT("esp/text", "Booting");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(100);
    }
  }

  client.publish("esp/text", "WEMOS started");
  int subcheck = client.subscribe("esp/control");
  Serial.print("subscribe = ");
  Serial.println(subcheck);
  client.subscribe("espcontrol");
  client.subscribe("esp/controlcurrent");
}
//________________________________________________________________________________

//                     callback ( ) - runs when MQTT message comes in
//      *******************************************************
void callback(char *intopic, byte *payload, unsigned int length)
{

  Serial.print("Message arrived in topic: ");
  Serial.println(intopic);

  String readString = "";
  for (int i = 0; i < length; i++)
  {
    delay(2); //delay to allow byte to arrive in input buffer
    char c = payload[i];
    readString += c;
  }
  // Serial.print("Message:");
  // Serial.println(readString);

  if (strcmp(intopic, "esp/control") == 0)
  { //0 is true
    switch (readString.toInt())
    { // Node red sends intigers to topic esp/control from dash buttons
    case 1:
    {
      sendMQTT("esp/text", "enabled = 1");
    }
    break;
    case 2:
    {
      sendMQTT("esp/text", "enabled = 0");
    }
    break;
    case 3:
    {
      sendMQTT("esp/text", "Serial print turned on"); //serial on
      quickprint_flag = 1;
    }
    break;
    case 4:
    {
      sendMQTT("esp/text", "Serial print turned off"); // serial off
      quickprint_flag = 0;
    }
    break;
    case 5:
      sendMQTT("esp/text", "Reset Ah");

      break;
    case 6:

      break;
    case 7:
      if (sendSig_Flag == 1)
        sendSig_Flag = 0;
      else if (sendSig_Flag == 0)
        sendSig_Flag = 1;
      sendMQTT("esp/text", String(sendSig_Flag));
      break;
    case 8:
    {
      sendMQTT("esp/text", "Restarting ESP");
      delay(10);
      ESP.restart();
    }
    break;
    }
  }
  if (strcmp(intopic, "esp/controlcurrent") == 0)
  { //0 is true
    //    currentTarget = readString.toInt();
    Serial.println(readString.toInt());
    Serial.println(intopic);
  }
}
//________________________________________________________________________________

//                     sendMQTT ( )
//      *******************************************************
void sendMQTT(String topic, String message)
{
  char pubString[message.length() + 1];
  // make array message size plus /n
  message.toCharArray(pubString, message.length() + 1);
  char sendtopic[topic.length() + 1];
  topic.toCharArray(sendtopic, topic.length() + 1);
  //publish the data to MQTT
  client.publish(sendtopic, pubString);
}
//___________________________________________________________

// Quick print function, easyway to print to serial
//      *******************************************************
void qp(String to_print, int linefeed)
{
  if (linefeed == 0 && quickprint_flag == 1)
    Serial.print(to_print);
  else if (linefeed == 1 && quickprint_flag == 1)
    Serial.println(to_print);
}
//________________________________________________________________________________

//                     send signalk data over UDP
//      *******************************************************
void sendSigK(String sigKey, float data)
{

  if (sendSig_Flag == 1)
  {
    DynamicJsonBuffer jsonBuffer;
    String deltaText;

    //  build delta message
    JsonObject &delta = jsonBuffer.createObject();

    //updated array
    JsonArray &updatesArr = delta.createNestedArray("updates");
    JsonObject &thisUpdate = updatesArr.createNestedObject();   //Json Object nested inside delta [...
    JsonArray &values = thisUpdate.createNestedArray("values"); // Values array nested in delta[ values....
    JsonObject &thisValue = values.createNestedObject();
    thisValue["path"] = sigKey;
    thisValue["value"] = data;

    thisUpdate["Source"] = "ESP32";

    // Send UDP packet
    Udp.beginPacket(remoteIp, remotePort);
    delta.printTo(Udp);
    Udp.println();
    Udp.endPacket();
    delta.printTo(Serial);
    Serial.println();
  }
} //___________________________________________________________

//                     send sensor data
//      *******************************************************
void send_Data(int send_Data_Rate)
{

  //is it time to process the next state code?
  if (currentMillis - timer1_Millis < send_Data_Rate)
  {
    //No, it is not time
    return;
  }

  if (currentMillis - timer1_Millis > send_Data_Rate)
  {
    //Yes, it is now time

    // Pressure ---------------------------------------------------------------
    float BMEpressure = BE280.readFloatPressure();
    float BMEtemp = BE280.readTempC();

    sendSigK("environment.outside.pressure", BMEpressure);
    sendSigK("environment.inside.temperature", (BMEtemp + 273.15));
    lcd.setCursor(0, 0);
    lcd.print("Pressure=");
    lcd.print(BMEpressure / 100);
    lcd.setCursor(0, 1);
    lcd.print("Temp=");
    lcd.print(BMEtemp);

    qp("Pressure:", 0);
    qp(String(BMEpressure), 1);
    qp("Temp:", 0);
    qp(String(BMEtemp), 1);

    // Current ---------------------------------------------------------------
    float current_mA = 0;
    current_mA = ina219.getCurrent_mA();
    qp("Current mA:", 0);
    qp(String(current_mA), 1);
    sendMQTT("esp/current", String(current_mA));
    sendSigK("esp.meter.current", current_mA);

    // Voltage ---------------------------------------------------------------
    float multiplier = 0.1875F; /* ADS1115  @ +/- 6.144V gain (16-bit results) */
    int16_t results;
    results = ads2.readADC_Differential_0_1();
    sendMQTT("esp/voltage", String(results * multiplier));
    Serial.print("Differential: ");
    Serial.print(results);
    Serial.print("(");
    Serial.print(results * multiplier);
    Serial.println("mV)");
    Serial.print("MQTT client connected:  ");
    Serial.println(client.connected());
    counter += 1;
    String counterTime = String(counter / 60 / 60) + ":" + String(counter / 60 % 60) + ":" + String(counter % 60);
    Serial.print("Counter = ");
    Serial.println(counterTime);
    sendMQTT("esp/pulseWidth", counterTime);
    Serial.print("Rebooted=");
    Serial.println(rebootedString);

    if (client.connected() != true)
    {
      Serial.println("Rebooting from data sender module");
      LogOn();
      rebootedString = String(rebooted + " - " + counterTime);
      sendMQTT("esp/reboot", rebootedString);
      rebooted += 1;
    }

    timer1_Millis = currentMillis; //reset timing
  }

} //___________________________________________________________

void handle_OnConnect()
{
  //LED1status = LOW;
  //LED2status = LOW;
  //Serial.println("GPIO4 Status: OFF | GPIO5 Status: OFF");
  server.send(200, "text/html", SendHTML(LED1status));
}

void handle_led1on()
{
  LED1status = HIGH;
  Serial.println("GPIO4 Status: ON");
  server.send(200, "text/html", SendHTML(LED1status));
}

void handle_led1off()
{
  LED1status = LOW;
  Serial.println("GPIO4 Status: OFF");
  server.send(200, "text/html", SendHTML(LED1status));
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

String SendHTML(uint8_t led1stat)
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>LED Control</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #3498db;}\n";
  ptr += ".button-on:active {background-color: #2980b9;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>ESP32 Web Server</h1>\n";
  ptr += "<h3>Using Access Point(AP) Mode</h3>\n";

  if (led1stat)
  {
    ptr += "<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n";
  }
  else
  {
    ptr += "<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/led1on\">ON</a>\n";
  }

  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}
