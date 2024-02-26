#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <math.h>
#include <U8g2lib.h>
#include <ir_Gree.h>
#include <NTPClient.h>

#include "main.h"
#include "ui.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// Global Variable
const int OLED_ckl = D3;
const int OLED_sdin = D2;
const int OLED_rst = D1;
const int OLED_cs = D0;
WiFiClient espClient;
WiFiManager wifiManager;
//PubSubClient client(espClient);
IRGreeAC ac(IR_pin);
IRsend irsend(IR_pin);
U8G2_SSD1322_NHD_256X64_1_3W_SW_SPI u8g2(U8G2_R2, /* clock=*/OLED_ckl, /* data=*/OLED_sdin, /* cs=*/OLED_cs, /* reset=*/OLED_rst); // OLED Display


// Set up the NTP UDP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

//Declare prototype functions
void increment();
void decrement();
void update();
void updateServerValue();

void btnSwingInt()
{
  isSwing = !isSwing;
  update();
  //updateServerValue();
}

void btnSpeedInt()
{
  fanSpeed = ++fanSpeed % 4;
  update();
  //updateServerValue();
}

// function for up-down interrupt
void IRAM_ATTR btnUpInt()
{
  currentAction = ACTION_UP;
}

void IRAM_ATTR btnDnInt()
{
  currentAction = ACTION_DOWN;
}

void changeACmode()
{
  isCool = !isCool;
  update();
  //updateServerValue();
}

void updateDisplay()

{

  u8g2.firstPage();

  if (isOn)
  {

    char currentTemp[3];
    String(visTemp).toCharArray(currentTemp, 3);
    //Serial.print("Visible Temp: ");
    //Serial.println(visTemp);

    //Serial.print("Actual Temp Temp: ");
    //Serial.println(setTemp);
    
    do
    {
      u8g2.drawBox(205, 4, 2, 55);

      u8g2.drawXBMP(211, 4, 29, 13, wifi);
      
      u8g2.drawXBMP(217, 45, 19, 15, bulb);

      if (isCool)
      {
        u8g2.setFont(u8g2_font_glasstown_nbp_tf);
        u8g2.drawStr(0, 17, "SET");
        u8g2.setFont(u8g2_font_logisoso50_tn);
        u8g2.drawStr(13, 57, currentTemp);
        u8g2.setFont(u8g2_font_helvR24_tf);
        u8g2.drawUTF8(80, 57, "°c");
        u8g2.drawXBMP(189, 5, 13, 12, cool);
      }
      else
      {
        u8g2.drawStr(0, 30, "COOL OFF");
      }

      if (isSwing)
        u8g2.drawXBMP(187, 24, 13, 13, swing_on);
      else
        u8g2.drawXBMP(187, 24, 13, 13, swing_off);

      switch (fanSpeed)
      {
      case 0:
        u8g2.drawXBMP(161, 44, 39, 15, fan_auto);
        break;
      case 1:
        u8g2.drawXBMP(161, 44, 39, 15, fan_min);
        break;
      case 2:
        u8g2.drawXBMP(161, 44, 39, 15, fan_mid);
        break;
      case 3:
        u8g2.drawXBMP(161, 44, 39, 15, fan_max);
        break;
      }

      // u8g2.drawXBMP(0,0, u8g2_logo_97x51_width, u8g2_logo_97x51_height, u8g2_logo_97x51_bits);

      // u8g2.setFont(u8g2_font_ncenB10_tr);
      // u8g2.drawStr(0,12,"On");
    } while (u8g2.nextPage());
  }
  else
  {
    // Display Clock when AC is off
    timeClient.update();
    unsigned long epoch = timeClient.getEpochTime();

    String formattedDate = timeClient.getFormattedTime();
    //Serial.println(formattedDate);

    // Extract date
    int splitT = formattedDate.indexOf("T");
    String dayStamp = formattedDate.substring(0, splitT);
    //Serial.print("DATE: ");
    //Serial.println(dayStamp);
    // Extract time
    String timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 3);
    //Serial.print("HOUR: ");
    //Serial.println(timeStamp);

    do
    {
      u8g2.setFont(u8g2_font_logisoso50_tn);
      u8g2.drawStr(38, 57, timeStamp.c_str());
    } while (u8g2.nextPage());
  }
}

void setOn() {
    ac.on();
}

void setOff() {
    ac.off();
}

void setLight() {
  ac.setLight(false);
}

void setCoolMode() {
  ac.setMode(kGreeCool);
}

void setACTemp(const uint8_t desired) {
  ac.setTemp(setTemp);
}

void send() {
  ac.send();
  updateDisplay();
}

void update()
{
  static bool isLastOn = false;
  static int lastTemp = 0;
  static bool lastIsSwing = isSwing;
  static int lastFanSpeed = fanSpeed;
  static int lastIsCool = isCool;
  if (visTemp == lastTemp && isSwing == lastIsSwing && lastFanSpeed == fanSpeed && lastIsCool == isCool && isLastOn == isOn)
    return;

  //Display is always off;
  setLight();
  if (isOn)
  {
    //Serial.println("Power On");  
    setOn();

    if (isCool)
    {
      Serial.println("Send IR : temp = " + String(setTemp) + " swing = " + String(isSwing) + " Fan Speed : " + String(fanSpeed) + " IsLastOn : " + isLastOn);
      isLastOn = true;
      lastTemp = setTemp;
      lastIsSwing = isSwing;
      lastFanSpeed = fanSpeed;
      lastIsCool = true;

      //Set and send commands
      setCoolMode();
      //ac.setSwing(isSwing);
      setACTemp(setTemp);
    }
    else
    {
      //ac.setMode(kGreeFan);
      lastIsCool = false;
    }

    send();
  }
  else if (isLastOn)
  {
    //Serial.println("Send off");
    isLastOn = false;
    //Serial.println("Power Off");  

    //set and send command
    setOff();
    send();
    
    //irsend.sendRaw(ac_off, 349, 38); //I need to send raw for off because Library didn't work.
  }

  //Turn on Display
  //Serial.println("UpdateDisplay");
  //previousMillis = millis();
  //currentContrast = 255;
  //u8g2.setContrast(currentContrast);

  //updateDisplay();
}
void blink()
{
  //Blink on received MQTT message
  digitalWrite(2, LOW);
  delay(20);
  digitalWrite(2, HIGH);
}

void setup_ota()
{
  // Set OTA Password, and change it in platformio.ini
  ArduinoOTA.setHostname("ESP8266-AC");
  // ArduinoOTA.setPassword("12345678");
  ArduinoOTA.onStart([]() {
    do
    {
      u8g2.setFont(u8g2_font_helvR14_tr);
      u8g2.drawStr(0, 14, "OTA updating...");
    } while (u8g2.nextPage());
  });
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
  ArduinoOTA.onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR)
      ; // Auth failed
    else if (error == OTA_BEGIN_ERROR)
      ; // Begin failed
    else if (error == OTA_CONNECT_ERROR)
      ; // Connect failed
    else if (error == OTA_RECEIVE_ERROR)
      ; // Receive failed
    else if (error == OTA_END_ERROR)
      ; // End failed
  });
  ArduinoOTA.begin();
}

void ACOnOff()
{
  if (!isOn)
  {
    //Serial.println("AC On");
    isOn = true;
  }
  else
  {
    //Serial.println("AC Off");
    isOn = false;
  }
  update();
}

void reconnect()
{
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
  
}

void increment()
{
  if (!isCool)
    return;
  if (visTemp >= setTemp) {
    setTemp++;  
  }
  visTemp++;
  update();
  //updateServerValue();
}

void decrement()
{
  if (!isCool)
    return;

  if(setTemp > minTemp) {
    setTemp--;
  }
  visTemp--;
  update();
  //updateServerValue();
}

void power()
{
  ACOnOff();
  //updateServerValue();
}

void IRAM_ATTR btnPowerPressed()
{
  // int press_millis = millis();
  // while (digitalRead(btnPower) == LOW)
  // {
  //   delay(0);
  //   if (millis() - press_millis > 1000)   //if long press, change AC Mode
  //   {
  //     isCool = !isCool;
  //     update();
  //     return;
  //   }
  // };
  currentAction = ACTION_POWER;
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.println("Callback?");
  char c_payload[length];
  memcpy(c_payload, payload, length);
  c_payload[length] = '\0';

  String s_topic = String(topic);
  String s_payload = String(c_payload);

  Serial.println(s_payload + "\0");

  StaticJsonBuffer<200> jsonBuffer;

  JsonObject &root = jsonBuffer.parseObject(s_payload);

  const char *name = root["name"];

  // Serial.println(name);
  if (strcmp(name, device_name.c_str()) != 0 && strcmp(name, device_name_secondary.c_str()) != 0)
  {
    return;
  }

  // blink();
  const char *characteristic = root["characteristic"];

  if (strcmp(characteristic, "CoolingThresholdTemperature") == 0)
  {
    int value = root["value"];
    if (value < minTemp || value > maxTemp)
    {
      return;
    }
    setTemp = value;
    update();
  }
  if (strcmp(characteristic, "Active") == 0)
  {
    int value = root["value"];
    if (value != 1 && value != 0)
    {
      return;
    }
    isOn = value;
    update();

    //set secondary device status (For primitive service as regular fan.)
    char data[150];
    String message = "{\"name\" : \"" + device_name_secondary + "\", \"service_name\" : \"" + service_name_secondary + "\", \"characteristic\" : \"On\", \"value\" : " + String(isOn) + "}";
    message.toCharArray(data, (message.length() + 1));
  }
  if (strcmp(characteristic, "On") == 0)
  {
    int value = root["value"];
    if (value != 1 && value != 0)
    {
      return;
    }
    isOn = value;
    update();

    //set primary device status (For Homekit HeaterCooler service)
    char data[150];
    String message = "{\"name\" : \"" + device_name + "\", \"service_name\" : \"" + service_name + "\", \"characteristic\" : \"Active\", \"value\" : " + String(isOn) + "}";
    message.toCharArray(data, (message.length() + 1));
  }

  if (strcmp(characteristic, "SwingMode") == 0)
  {
    int value = root["value"];
    if (value != 1 && value != 0)
    {
      return;
    }
    isSwing = value;
    update();
  }
  if (strcmp(characteristic, "RotationSpeed") == 0)
  {
    int value = root["value"];
    if (value < 0 || value > 3)
    {
      return;
    }
    fanSpeed = value;
    update();
  }
  /*
    // The value property of TargetHeaterCoolerState must be one of the following:
  Characteristic.TargetHeaterCoolerState.AUTO = 0;
  Characteristic.TargetHeaterCoolerState.HEAT = 1;
  Characteristic.TargetHeaterCoolerState.COOL = 2;
  */
  if (strcmp(characteristic, "TargetHeaterCoolerState") == 0)
  {
    int value = root["value"];
    if (value < 0 || value > 3)
    {
      return;
    }
    if (value == 0 || value == 2)
    {
      isCool = true;
    }
    else
    {
      isCool = false;
    }
    update();
  }

  //updateServerValue();
}

void setup()
{
  Serial.begin(9600);

  //Setup display
  Serial.println("Setting up display");
  u8g2.begin();
  u8g2.setFont(u8g2_font_logisoso50_tn);
  u8g2.firstPage();
  do
  {
    u8g2.drawStr(20, 17, "WiFi - AC");
  } while (u8g2.nextPage());

  // Setup buttons
  Serial.println("Setting up buttons");
  pinMode(btnPower, INPUT_PULLUP);
  pinMode(btnUp, INPUT_PULLUP);
  pinMode(btnDn, INPUT_PULLUP);

  pinMode(2, OUTPUT);

  // Setup networking
  Serial.println("Setting up network");
  WiFi.begin(ssid,password);
  //wifiManager.setConfigPortalTimeout(120);
  //wifiManager.autoConnect(autoconf_ssid, autoconf_pwd);
  //setup_ota();
  //client.setServer(mqtt_server, mqtt_port);
  //client.setCallback(callback);

  //Attach interrupt for manual button controls
  Serial.println("Attaching Interupt");
  attachInterrupt(digitalPinToInterrupt(btnPower), btnPowerPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(btnUp), btnUpInt, FALLING);
  attachInterrupt(digitalPinToInterrupt(btnDn), btnDnInt, FALLING);

  digitalWrite(2, HIGH);
  updateDisplay();
  //updateServerValue();

  //Setup IR Lib
  Serial.println("Setting up IR Lib");
  ac.begin();
  irsend.begin();

  //Start the NTP UDP client
  Serial.println("Setting up NTP Client");
  timeClient.begin();

  power();

  Serial.println("Setup done!");
}

void handleCurrentAction()
{
  switch (currentAction)
  {
    case ACTION_UP:
    {
      Serial.println("Action Up");
      increment();
      break;
    }
    case ACTION_DOWN:
    {
      Serial.println("Action Down");
      decrement();
      break;
    }
    case ACTION_POWER:
    {
      Serial.println("Action Power");
      power();
      break;
    }
    default:
    {
      break;
    }
  }

  currentAction = 0;
}

void loop()
{
  /*
  if (!client.connected() && (millis() - lastConnectingTime > 60000 ))
  {
    Serial.println("reconnect");
    reconnect();
    updateDisplay();
    lastConnectingTime = millis();
  }*/
  //client.loop();
  //ArduinoOTA.handle();
  //autoAdjustScreenBrightness();
  handleCurrentAction();
}