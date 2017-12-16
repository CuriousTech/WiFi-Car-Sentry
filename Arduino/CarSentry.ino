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

//uncomment to enable Arduino IDE Over The Air update code
#define OTA_ENABLE

//#define DEBUG
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
#ifdef OTA_ENABLE
#include <FS.h>
#include <ArduinoOTA.h>
#endif
#include <OneWire.h>
#include <TimeLib.h> // http://www.pjrc.com/teensy/td_libs_Time.html
#include <UdpTime.h>
#include <DHT.h>  // http://www.github.com/markruys/arduino-DHT
#include "eeMem.h"
#include <JsonClient.h> // https://github.com/CuriousTech/ESP8266-HVAC/tree/master/Libraries/JsonParse

const char controlPassword[] = "password";    // device password for modifying any settings
const int serverPort = 81;                    // HTTP port

#define IN1       0  // Sink input 1 (Direct sink 5V tolerant)
#define OUT3      1  // TX Sink output 3 MOSFET (high to sink)
#define OUT1      2  // Sink output 1 MOSFET (low to sink) (Note: Rev 1 has P-channel here)
#define ESP_LED   2  // Blue LED on ESP07 (on low)
#define IN2       3  // RX 1~40V input (sink)
#define SDA       4  // OLED
#define SCL       5  // OLED
#define DHT22IO  12  // The DHT22
#define IN12V    13  // 1~30V input (sink) use PULLUP
#define VOLTFET  14  // voltage reading MOSFET
#define OUT2     15  // Sink output 2 MOSFET (high to sink)
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
uint16_t ssCnt = 10;
const char hostName[] = "CarSentry";
bool bKeyGood;
uint32_t sleepDelay = 10; // default value for WebSocket close -> sleepTimer
uint32_t sleepTimer = 60; // seconds delay after startup to enter sleep (Note: even if no AP found)
int8_t openCnt;
uint8_t nPulse;
uint8_t pulseCh;
IPAddress WSIP;

void jsonCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue);
JsonClient jsonParse(jsonCallback);
JsonClient jsonPush(jsonCallback);
void locCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue);
JsonClient locator(locCallback);

const char days[7][4] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
const char months[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

String dataJson()
{
  String s = "{";
  s += "\"t\": ";  s += now() - ((ee.tz + utime.getDST()) * 3600);
  s += ", \"ct\": ";  s += sDec(carTemp);
  s += ", \"rh\": ";   s += sDec(rh);
  s += ", \"v\": ";   s += (float)volts / 100;
  s += ", \"in1\": ";   s += digitalRead(IN1) ? 0:1;
  s += ", \"in2\": ";   s += digitalRead(IN2) ? 0:1;
  s += ", \"in12v\": ";   s += digitalRead(IN12V) ? 0:1;
  s += ", \"lat\": \"";   s += ee.szLat;
  s += "\", \"lon\": \"";   s += ee.szLon;
  s += "\"}";
  return s;
}

String setJson() // settings
{
  String s = "{";
  s += "\"o\":";    s += ee.bEnableOLED;
  s += ",\"tz\":";  s += ee.tz;
  s += ", \"gt\": ";   s += ee.get_time;
  s += ", \"gl\": ";   s += ee.get_loc;
  s += ",\"o1\":";  s += digitalRead(OUT1) ? 0:1;
  s += ",\"o2\":";  s += digitalRead(OUT2) ? 1:0;
  s += ",\"o3\":";  s += digitalRead(OUT3) ? 1:0;
  s += ",\"to\": "; s += ee.time_off;
  s += ", \"ro\": ";   s += ee.roaming;
  s += ",\"d\": \""; s += ee.szDomain;
  s += "\"}";
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
  String s = dataJson();
  events.send(s.c_str(), "state");
  ws.textAll(String("state;") + s);
  ssCnt = 58;
}

const char *jsonListCmd[] = { "cmd",
  "key",
  "oled",
  "TZ", // location
  "TO", // time_off
  "sleep",
  "O1", // OUT1
  "O2",
  "O3",
  "PLS", // Pulse OUTn for 1 second
  "GT", // get time
  "GL", // get location
  "HI", // host IP/port
  "SD", // set domain
  "RO", // roaming
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
        case 5: // O1=n
          digitalWrite(OUT1, iValue ? LOW:HIGH);
          break;
        case 6: // O2=n
          digitalWrite(OUT2, iValue ? HIGH:LOW);
          break;
        case 7: // O3=n
          digitalWrite(OUT3, iValue ? HIGH:LOW);
          break;
        case 8: // PLS=Ch
          nPulse = 2;
          pulseCh = iValue;
          break;
        case 9: // GT
          ee.get_time = iValue ? true:false;
          break;
        case 10: // GL
          ee.get_loc = iValue ? true:false;
          break;
        case 11: // host IP / port  (call from host with ?h=80)
          ee.hostIP = WSIP;
          ee.hostPort = iValue ? iValue:80;
          break;
        case 12: // global domain
          strncpy(ee.szDomain, psValue, sizeof(ee.szDomain));
          break;
        case 13: // RO
          ee.roaming = iValue ? true:false;
          break;
      }
      break;
  }
}

const char *jsonListLoc[] = { "data",
  "result",
  "lat",
  "lon",
  "time",
  NULL
};
      
void locCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue)
{
  switch(iEvent)
  {
    case 0: // cmd
      switch(iName)
      {
        case 0: // result
          break;
        case 1: // lat
          strncpy(ee.szLat, psValue, sizeof(ee.szLat) );
          ee.locRoam = wifi.isSecure();
          break;
        case 2: // lon
          strncpy(ee.szLon, psValue, sizeof(ee.szLon) );
          break;
        case 3: // time
          utime.set(iValue, ee.tz);
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
      WSIP = client->remoteIP();
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
  digitalWrite(OUT1, HIGH);
  pinMode(OUT1, OUTPUT);
  pinMode(OUT2, OUTPUT);

  // initialize dispaly
  display.init();
  display.flipScreenVertically();

#ifdef DEBUG
  Serial.begin(115200);
  delay(3000);
  Serial.println();
#endif

  WiFi.hostname(hostName);
  if(!wifi.autoConnect(hostName, controlPassword, ee.roaming)) // Tries config AP.  goes to DeepSleep if not found
  {
    if(digitalRead(IN1) == LOW) // hold the flash button down a couple seconds after powerup to clear SSID and password
    {                           // don't use this if IN1 is connected to something
      ee.szSSID[0] = 0;
      ee.szSSIDPassword[0] = 0;
      eemem.update();
#ifdef DEBUG
      Serial.println("SSID cleared");
      delay(1000);
#endif
    }
    uint32_t us = 1000000; // 1 second to restart
    display.displayOff();
    ESP.deepSleep(us, WAKE_RF_DEFAULT);
  }

  if(wifi.isCfg() == false)
  {
#ifdef DEBUG
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
#endif
    MDNS.begin ( hostName, WiFi.localIP() );
  }

  MDNS.addService("http", "tcp", serverPort);

#ifdef USE_SPIFFS
  SPIFFS.begin();
  server.addHandler(new SPIFFSEditor("admin", controlPassword));
#endif

  attachInterrupt(IN1, in1ISR, FALLING);
  attachInterrupt(IN12V, in12VISR, CHANGE);

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

  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "[";
    int n = WiFi.scanComplete();
    if(n == -2){
      WiFi.scanNetworks(true);
    } else if(n){
      for (int i = 0; i < n; ++i){
        if(i) json += ",";
        json += "{";
        json += "\"rssi\":"+String(WiFi.RSSI(i));
        json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
        json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
        json += ",\"channel\":"+String(WiFi.channel(i));
        json += ",\"secure\":"+String(WiFi.encryptionType(i));
        json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
        json += "}";
      }
      WiFi.scanDelete();
      if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
      }
    }
    json += "]";
    request->send(200, "text/json", json);
    json = String();
  });

  server.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  });
  server.begin();

#ifdef OTA_ENABLE
  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    eemem.update();
  });
#endif

  dht.setup(DHT22IO, DHT::DHT22);
  jsonParse.addList(jsonListCmd);

  if(wifi.isCfg() == false) // connected to the net
  {
    if(ee.get_time)
      utime.start();
    String sUri = String("/car?lon=\"");
    sUri += ee.szLon;
    sUri += "\"&lat=\"";
    sUri += ee.szLat;
    sUri += "\"";
    IPAddress ip(ee.hostIP);
    String url;
    if(wifi.isSecure())
      url = ip.toString();
    else
      url = ee.szDomain;
    jsonPush.begin(url.c_str(), sUri.c_str(), ee.hostPort, false, false, NULL, NULL);
    jsonPush.addList(jsonListCmd);
    if(ee.szLat[0] == 0 || ee.get_loc || !wifi.isSecure() || ee.locRoam) // only needed first time and when roaming, or was roaming last
      updateLocation();
  }

  sleepTimer = 60;
  ssCnt = 10; // reset to 10 seconds
}

void loop()
{
  static uint8_t hour_save, min_save, sec_save;
  static bool bLastOn;

  MDNS.update();
#ifdef OTA_ENABLE
  ArduinoOTA.handle();
#endif

  if(!wifi.isCfg() && ee.get_time)
    utime.check(ee.tz);

  if(bIn1Triggered) // sink triggered
  {
    bIn1Triggered = false;
//    Serial.println("IN1 triggered");
    events.send("IN1 triggered", "alert");
  }

  if(bIn12VTriggered) // 12V in changed
  {
    bIn12VTriggered = false;
//    Serial.print("12V changed to ");
//    Serial.println( digitalRead(IN12V) );
    sendState();
  }

  if(sec_save != second()) // only do stuff once per second
  {
    sec_save = second();

    static uint8_t dht_cnt = 1;
    if(--dht_cnt == 0)
    {
      dht_cnt = 5;
      uint16_t r = (uint16_t)(dht.getHumidity() * 10);
      if(dht.getStatus() == DHT::ERROR_NONE)
      {
        rh = r;
        carTemp = (dht.toFahrenheit(dht.getTemperature()) * 10);
      }//else Serial.println("Temp error");
    }

    if(--ssCnt == 0) // periodic data update
    {
       sendState();
    }

    switch(nPulse)
    {
      case 1:
        switch(pulseCh)
        {
          case 1: digitalWrite(OUT1, HIGH); break;
          case 2: digitalWrite(OUT2, LOW); break;
          case 3: digitalWrite(OUT3, LOW); break;
        }
        nPulse = 0;
        break;
      case 2:
        switch(pulseCh)
        {
          case 1: digitalWrite(OUT1, LOW); break;
          case 2: digitalWrite(OUT2, HIGH); break;
          case 3: digitalWrite(OUT3, HIGH); break;
        }
        nPulse = 1;
        break;
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

    if(sleepTimer && openCnt == 0 && wifi.isCfg() == false) // don't sleep until all ws connections are closed
    {
      if(--sleepTimer == 0)
      {
        if(ee.time_off)
        {
          eemem.update(); // Update EE if anything changed before entering sleep
          delay(100);

//        uint32_t us = ee.time_off * 1000000 * 60;  // minutes to us
          uint32_t us = ee.time_off * 60022000;  // minutes to us adjusted
          display.displayOff();
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
    display.drawString(2, 52, String((float)rh / 10, 1) + "%");
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

#define min(a,b) ((a)<(b)?(a):(b))

void updateLocation()
{
  int n = min(WiFi.scanNetworks(false,true), 5);
  if (n <= 0) return;

  String multiAPString = "";
  for(int i = 0; i < n; i++)
  {
    if(i > 0)
      multiAPString += ",";
     multiAPString += WiFi.BSSIDstr(i) + "," + WiFi.RSSI(i);
  }
  char multiAPs[multiAPString.length() + 1];
  multiAPString.toCharArray(multiAPs, multiAPString.length());
  multiAPString = "/wifi?v=1.1&search=";
  multiAPString += encodeBase64(multiAPs, multiAPString.length());
  locator.begin("api.mylnikov.org", multiAPString.c_str(), 80, false, false, NULL, NULL);
  locator.addList(jsonListLoc);
}

/*
 * Base64 code by Rene Nyfenegger:
 * http://www.adp-gmbh.ch/cpp/common/base64.html
 */
String base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String encodeBase64(char* bytes_to_encode, unsigned int in_len)
{
  String ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }
  return ret;
}
