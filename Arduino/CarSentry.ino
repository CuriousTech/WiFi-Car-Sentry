/**The MIT License (MIT)

Copyright (c) 2017 by Greg Cunningham, CuriousTech

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
//#define USE_SPIFFS

#include <Wire.h>
#include <ssd1306_i2c.h> // https://github.com/CuriousTech/WiFi_Doorbell/tree/master/Libraries/ssd1306_i2c
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include "WiFiManager.h"
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#ifdef USE_SPIFFS
#include <FS.h>
#include <SPIFFSEditor.h>
#else
#include "pages.h"
#endif
#include <OneWire.h>
#include <TimeLib.h> // http://www.pjrc.com/teensy/td_libs_Time.html
#include <UdpTime.h>
#include <DHT.h>  // http://www.github.com/markruys/arduino-DHT
#include "eeMem.h"
#include <JsonParse.h> // https://github.com/CuriousTech/ESP8266-HVAC/tree/master/Libraries/JsonParse

const char controlPassword[] = "password";    // device password for modifying any settings
const int serverPort = 80;                    // HTTP port

#define IN1       0  // sink input 1
#define OUT1      2  // sink output 1 MOSFET
#define ESP_LED   2  // Blue LED on ESP07 (on low)
#define SDA       4
#define SCL       5  // OLED
#define DHT22IO  12  // The DHT22
#define IN12V    13  // 12V input (sink) use PULLUP
#define VOLTFET  14  // voltage reading MOSFET
#define OUT2     15  // Sink output 2 MOSFET
#define SLEEP    16  // Wake pin

uint32_t lastIP;

SSD1306 display(0x3C, SDA, SCL); // Initialize the oled display for address 0x3C, SDA=4, SCL=5 (is the OLED mislabelled?)
int displayOnTimer;

eeMem eemem;
DHT dht;
UdpTime utime;

WiFiManager wifi;  // AP page:  192.168.4.1
AsyncWebServer server( serverPort );
AsyncEventSource events("/events"); // event source (Server-Sent events)
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws

uint16_t rh;
int16_t carTemp;
uint16_t volts;
uint16_t ssCnt = 60;
const char hostName[] = "CarSentry";
bool bKeyGood;
uint32_t sleepDelay = 10; // default value for WebSocket close -> sleepTimer
uint32_t sleepTimer = 60; // seconds delay after startup to enter sleep (Note: even if no AP found)
int8_t openCnt;

void jsonCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue);
JsonParse jsonParse(jsonCallback);

const char days[7][4] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
const char months[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

String dataJson()
{
  String s = "{";
  s += "\"t\": ";  s += now() - ((ee.tz + utime.getDST()) * 3600);
  s += ", \"ct\": ";  s += sDec(carTemp);
  s += ", \"rh\": ";   s += sDec(rh);
  s += ", \"v\": ";   s += (float)volts / 100;
  s += ", \"on\": ";   s += digitalRead(IN12V) ? 0:1;
  s += "}";
  return s;
}

String setJson() // settings
{
  String s = "{";
  s += "\"o\":";    s += ee.bEnableOLED;
  s += ",\"tz\":";  s += ee.tz;
  s += ",\"o1\":";  s += digitalRead(OUT1);
  s += ",\"o2\":";  s += digitalRead(OUT2);
  s += ",\"to\": "; s += ee.time_off;
  s += "}";
  return s;
}

void parseParams(AsyncWebServerRequest *request)
{
  static char temp[256];
  char password[64];
  int val;

 if(request->params() == 0)
    return;

  // get password first
  for ( uint8_t i = 0; i < request->params(); i++ ) {
    AsyncWebParameter* p = request->getParam(i);
    p->value().toCharArray(temp, 100);
    String s = wifi.urldecode(temp);
    switch( p->name().charAt(0)  )
    {
      case 'k': // key
        s.toCharArray(password, sizeof(password));
        break;
    }
  }

  uint32_t ip = request->client()->remoteIP();

  if(strcmp(controlPassword, password))
  {
    lastIP = ip;
    return;
  }

  lastIP = ip;

  for ( uint8_t i = 0; i < request->params(); i++ ) {
    AsyncWebParameter* p = request->getParam(i);
    p->value().toCharArray(temp, 100);
    String s = wifi.urldecode(temp);

    val = s.toInt();

    switch( p->name().charAt(0)  )
    {
      case 'h': // host IP / port  (call from host with ?h=80)
        ee.hostIP = ip;
        ee.hostPort = val ? val:80;
        break;
      case 't': // time off
        ee.time_off = val;
        break;
      case 's': // AP SSID
        s.toCharArray(ee.szSSID, sizeof(ee.szSSID));
        break;
      case 'p': // AP password
        wifi.setPass(s.c_str());
        break;
    }
  }
}

String sDec(int t) // just 123 to 12.3 string
{
  String s = String( t / 10 ) + ".";
  s += t % 10;
  return s;
}

// Time in hh:mm[:ss][AM/PM]
String timeFmt(bool do_sec, bool do_M)
{
  String r = "";
  if(hourFormat12() < 10) r = " ";
  r += hourFormat12();
  r += ":";
  if(minute() < 10) r += "0";
  r += minute();
  if(do_sec)
  {
    r += ":";
    if(second() < 10) r += "0";
    r += second();
    r += " ";
  }
  if(do_M)
  {
      r += isPM() ? "PM":"AM";
  }
  return r;
}

void handleS(AsyncWebServerRequest *request) { // standard params, but no page
  parseParams(request);

  String page = "{\"ip\": \"";
  page += WiFi.localIP().toString();
  page += ":";
  page += serverPort;
  page += "\"}";
  request->send ( 200, "text/json", page );
}

void onEvents(AsyncEventSourceClient *client)
{
  events.send(dataJson().c_str(), "state");
}

void sendState()
{
  events.send(dataJson().c_str(), "state");
  ws.printfAll("state;%s", dataJson().c_str());
  ssCnt = 60;
}

const char *jsonListCmd[] = { "cmd",
  "key",
  "oled",
  "TZ", // location
  "TO", // time_off
  "sleep",
  "O1", // OUT1
  "O2",
  NULL
};

void jsonCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue)
{
  if(!bKeyGood && iName) return; // only allow for key

  switch(iEvent)
  {
    case 0: // cmd
      switch(iName)
      {
        case 0: // key
          if(!strcmp(psValue, controlPassword)) // first item must be key
            bKeyGood = true;
          break;
        case 1: // OLED
          ee.bEnableOLED = iValue ? true:false;
          break;
        case 2: // TZ
          ee.tz = iValue;
          utime.start();
          break;
        case 3: // TO
          ee.time_off = iValue;
          break;
        case 4: // sleep
          sleepDelay = constrain(iValue, 1, 10000);
          break;
        case 5:
          digitalWrite(OUT1, iValue);
          break;
        case 6:
          digitalWrite(OUT2, iValue);
          break;
      }
      break;
  }
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{  //Handle WebSocket event

  switch(type)
  {
    case WS_EVT_CONNECT:      //client connected
      client->printf("state;%s", dataJson().c_str());
      client->printf("set;%s", setJson().c_str());
      client->ping();
      openCnt++;
      break;
    case WS_EVT_DISCONNECT:    //client disconnected
      if(openCnt)
        openCnt--;
      if(sleepDelay)
        sleepTimer = sleepDelay;
      break;
    case WS_EVT_ERROR:    //error was received from the other end
      break;
    case WS_EVT_PONG:    //pong message was received (in response to a ping request maybe)
      break;
    case WS_EVT_DATA:  //data packet
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if(info->final && info->index == 0 && info->len == len){
        //the whole message is in a single frame and we got all of it's data
        if(info->opcode == WS_TEXT){
          data[len] = 0;

          char *pCmd = strtok((char *)data, ";"); // assume format is "name;{json:x}"
          char *pData = strtok(NULL, "");
          if(pCmd == NULL || pData == NULL) break;
          bKeyGood = false; // for callback (all commands need a key)
          jsonParse.process(pCmd, pData);
        }
      }
      break;
  }
}

volatile bool bIn1Triggered;

void in1ISR()
{
  bIn1Triggered = true;
}

volatile bool bIn12VTriggered;

void in12VISR()
{
  bIn12VTriggered = true;
}

void setup()
{
  pinMode(IN1, INPUT_PULLUP);
  pinMode(IN12V, INPUT_PULLUP);
  pinMode(VOLTFET, OUTPUT);
  pinMode(OUT1, OUTPUT);
  pinMode(OUT2, OUTPUT);

  attachInterrupt(IN1, in1ISR, FALLING);
  attachInterrupt(IN12V, in12VISR, CHANGE);

  // initialize dispaly
  display.init();
  display.flipScreenVertically();

  Serial.begin(115200);
//  delay(3000);
  Serial.println();

  WiFi.hostname(hostName);
  wifi.autoConnect(hostName); // Tries config AP, then starts softAP mode for config
  if(wifi.isCfg() == false)
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  
    if ( !MDNS.begin ( hostName, WiFi.localIP() ) )
      Serial.println ( "MDNS responder failed" );
  }

  MDNS.addService("http", "tcp", serverPort);

#ifdef USE_SPIFFS
  SPIFFS.begin();
  server.addHandler(new SPIFFSEditor("admin", controlPassword));
#endif

  // attach AsyncEventSource
  events.onConnect(onEvents);
  server.addHandler(&events);
  // attach AsyncWebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on ( "/", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest *request) // Main webpage interface
  {
    parseParams(request);
    if(wifi.isCfg())
      request->send( 200, "text/html", wifi.page() ); // WIFI config page
    else
    {
    #ifdef USE_SPIFFS
      request->send(SPIFFS, "/index.htm");
    #else
      request->send_P(200, "text/html", page1);
    #endif
    }
  });
  server.on ( "/s", HTTP_GET | HTTP_POST, handleS );
  server.on ( "/set", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send( 200, "text/json", setJson() );
  });
  server.on ( "/json", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send( 200, "text/json", dataJson() );
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404);
  });
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  });
  server.begin();

  jsonParse.addList(jsonListCmd);

  if(wifi.isCfg() == false) // not really connected to the net yet
    utime.start();

  dht.setup(DHT22IO, DHT::DHT22);
}

void loop()
{
  static uint8_t hour_save, min_save, sec_save;
  static bool bLastOn;

  MDNS.update();

  if(!wifi.isCfg())
    utime.check(ee.tz);

  if(bIn1Triggered) // sink triggered
  {
    bIn1Triggered = false;
    Serial.println("IN1 triggered");
    events.send("IN1 triggered", "alert");
  }

  if(bIn12VTriggered) // 12V in changed
  {
    bIn12VTriggered = false;
    Serial.print("12V changed to ");
    Serial.println( digitalRead(IN12V) );
    sendState();
  }

  if(sec_save != second()) // only do stuff once per second
  {
    sec_save = second();

    static uint8_t dht_cnt = 3;
    if(--dht_cnt == 0)
    {
      dht_cnt = 5;
      uint16_t r = (uint16_t)(dht.getHumidity() * 10);
      if(dht.getStatus() == DHT::ERROR_NONE)
      {
        rh = r;
        carTemp = (dht.toFahrenheit(dht.getTemperature()) * 10);
      }
    }

    if(--ssCnt == 0) // periodic data update
    {
       sendState();
    }

    if(min_save != minute()) // only do stuff once per minute
    {
      min_save = minute();
      if (hour_save != hour()) // update time daily (at 2AM for DST)
      {
        if( (hour_save = hour()) == 2)
        {
          utime.start();
        }
      }
    }
    if(displayOnTimer)
    {
      displayOnTimer --;
    }

    // 100K / 6.34K to 1V : 17V = 1.008V or 1024 (17/1024=1.66)
    digitalWrite(VOLTFET, LOW);
    uint16_t v = analogRead(0);
    volts = (v * 1.582);
    digitalWrite(VOLTFET, HIGH);
//    Serial.print("volts ");
//    Serial.print(v);
//    Serial.print(" ");
//    Serial.println(volts);

    if(sleepTimer && openCnt == 0) // don't sleep until all ws connections are closed
    {
      if(--sleepTimer == 0)
      {
        if(ee.time_off)
        {
          eemem.update(); // Update EE if anything changed before entering sleep
          delay(100);

          uint32_t us = ee.time_off * 1000000;  // seconds to us
          ESP.deepSleep(us, WAKE_RF_DEFAULT);
        }
      }
    }
  }
  
  if(wifi.isCfg())
    return;

  // draw the screen here
  display.clear();

  if(ee.bEnableOLED || displayOnTimer) // draw only ON indicator if screen off
  {
    display.setFontScale2x2(false); // the small text

    String s;
  
    s = timeFmt(true, true); // the default scroller
    s += "  ";
    s += days[weekday()-1];
    s += " ";
    s += String(day());
    s += " ";
    s += months[month()-1];
    s += "  ";

    Scroller(s);

    display.drawString( 8, 22, "Temp");
    display.drawPropString(2, 33, String((float)carTemp / 10, 1) + "]" ); // <- that's a degree symbol
    display.drawString(2, 48, String((float)rh / 10, 1) + "%");
    display.drawString( 88, 22, "Volts");
    display.drawPropString(74, 33, String((float)volts / 100, 1) + "v" );
  }
  display.display();
}

// Text scroller optimized for very long lines
void Scroller(String s)
{
  static int16_t ind = 0;
  static char last = 0;
  static int16_t x = 0;

  if(last != s.charAt(0)) // reset if content changed
  {
    x = 0;
    ind = 0;
  }
  last = s.charAt(0);
  int len = s.length(); // get length before overlap added
  s += s.substring(0, 18); // add ~screen width overlap
  int w = display.propCharWidth(s.charAt(ind)); // measure first char
  if(w > 100) w = 10; // bug in function (0 returns 11637)
  String sPart = s.substring(ind, ind + 18);
  display.drawPropString(x, 0, sPart );

  if( --x <= -(w))
  {
    x = 0;
    if(++ind >= len) // reset at last char
      ind = 0;
  }
}
