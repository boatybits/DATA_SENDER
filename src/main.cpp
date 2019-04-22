#include <Arduino.h>
#include "myheader.h"

//*****************************************************************************************************************************************
//                         Run Setup
//*****************************************************************************************************************************************

void setup()
{
  Serial.begin(115200);
  Wire.begin(5, 4); // pins 5 & 4 for wemos
  LogOn();
  BE280.setI2CAddress(0x76); //Connect to BME on ox76
  if (BE280.beginI2C() == false)
    Serial.println("BE280 connect failed");
  else
    Serial.println("BE280 connected");
  delay(250); // let the BME settle down

  ina219.begin();
  ina219.setCalibration_16V_400mA();

  ads.setGain(GAIN_TWOTHIRDS);
  ads.begin();
}
//________________________________________________________________________________

//*****************************************************************************************************************************************
//                         Run Main Loop
//*****************************************************************************************************************************************
void loop()
{

  client.loop();
  currentMillis = millis();
  send_Data(1000);
}
//________________________________________________________________________________

//*****************************************************************************************************************************************
//*****************************************************************************************************************************************
//                         SUBROUTINES
//*****************************************************************************************************************************************
//*****************************************************************************************************************************************

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
//*******************************************************
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
//*******************************************************
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

// Quick print function
//*******************************************************
void qp(String to_print, int linefeed)
{
  if (linefeed == 0 && quickprint_flag == 1)
    Serial.print(to_print);
  else if (linefeed == 1 && quickprint_flag == 1)
    Serial.println(to_print);
}
//________________________________________________________________________________

//                     send signalk data over UDP
//*******************************************************
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
  }
}

//___________________________________________________________

//                     send sensor data
//*******************************************************
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
    Serial.print("Differential: ");
    Serial.print(results);
    Serial.print("(");
    Serial.print(results * multiplier);
    Serial.println("mV)");

    timer1_Millis = currentMillis; //reset timing
  }

} //                E n d  o f  s e n d D a t a ( )

//___________________________________________________________
