/**************************************************************
 * WiFiManager is a library for the ESP8266/Arduino platform
 * (https://github.com/esp8266/Arduino) to enable easy
 * configuration and reconfiguration of WiFi credentials and
 * store them in EEPROM.
 * inspired by http://www.esp8266.com/viewtopic.php?f=29&t=2520
 * https://github.com/chriscook8/esp-arduino-apboot
 * Built by AlexT https://github.com/tzapu
 * Licensed under MIT license
 **************************************************************/

#include "WiFiManager.h"
#include "ssd1306_i2c.h"
#include "icons.h"
#include "eeMem.h"

extern SSD1306 display;

WiFiManager::WiFiManager()
{
}

bool WiFiManager::autoConnect(char const *apName, const char *pPass, bool bAny) {
    _apName = apName;
    _pPass = pPass;

  DEBUG_PRINT("");
  DEBUG_PRINT("AutoConnect");
  bool bFound = false;
  int nOpen = -1;

  if(ee.szSSID[0]) // scan for configured AP
  {
    int n = WiFi.scanNetworks();
    if(n == 0 )
    {
      DEBUG_PRINT("No APs in range");
      return false;
    }
  
    for (int i = 0; i < n; i++)
    {
//    display.print(WiFi.SSID(i));

      if(WiFi.SSID(i) == ee.szSSID) // found cfg SSID
      {
        DEBUG_PRINT("SSID found.  Connecting.");
        bFound = true;
      }

      if(WiFi.encryptionType(i) == 7) // open
        nOpen = i;
    }
    // 2.3 seconds

    if(bFound == false)
    {
      if(bAny && nOpen >= 0)
      {
        DEBUG_PRINT("Connecting to open WiFi");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WiFi.SSID(nOpen).c_str());
        _secure = false;
        if ( hasConnected() )
        {
          _bCfg = false;
          return true;
        }
        return false; // can't connect to open AP  ~10 seconds
        
      }
      return false;
    }
    // connect to configured AP
    DEBUG_PRINT("Waiting for Wifi to connect");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ee.szSSID, ee.szSSIDPassword);
    _secure = true;
    if ( hasConnected() )
    {
      _bCfg = false;
      return true; // 1~9 seconds
    }
    return false; // can't find configured AP  ~10 seconds
  }

  //setup AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apName);
  DEBUG_PRINT("Started Soft Access Point");
  display.print("AP started:");
  IPAddress apIp = WiFi.softAPIP();
  display.print(apIp.toString());

  DEBUG_PRINT(WiFi.softAPIP());
  DEBUG_PRINT("Don't forget the port #");

  if (!MDNS.begin(apName))
    DEBUG_PRINT("Error setting up MDNS responder!");

  _timeout = true;
  _bCfg = true;
  return true;
}
bool WiFiManager::isSecure(void)
{
  return _secure;
}

bool WiFiManager::hasConnected(void)
{
  for(int c = 0; c < 50; c++)
  {
    if (WiFi.status() == WL_CONNECTED)
      return true;
    delay(200);
//    Serial.print(".");
    display.clear();
    display.drawXbm(34,10, 60, 36, WiFi_Logo_bits);
    display.setColor(INVERSE);
    display.fillRect(10, 10, 108, 44);
    display.setColor(WHITE);
    drawSpinner(4, c % 4);
    display.display();
  }
  DEBUG_PRINT("");
  DEBUG_PRINT("Could not connect to WiFi");
  display.print("No connection");
  return false;
}

bool WiFiManager::isCfg(void)
{
  return _bCfg;
}

void WiFiManager::setPass(const char *p){
  strncpy(ee.szSSIDPassword, p, sizeof(ee.szSSIDPassword) );
  eemem.update();
  DEBUG_PRINT("Updated EEPROM.  Restaring.");
  autoConnect(_apName, _pPass, false);
}

void WiFiManager::seconds(void) {
  static int s = 1; // do first list soon

  if(_timeout == false)
    return;
  if(--s)
    return;
  s = 60;
  int n = WiFi.scanNetworks(); // scan for stored SSID each minute
  if(n == 0 )
    return;

  for (int i = 0; i < n; i++)
  {
    display.print(WiFi.SSID(i));

    if(WiFi.SSID(i) == ee.szSSID) // found cfg SSID
    {
      DEBUG_PRINT("SSID found.  Restarting.");
      autoConnect(_apName, _pPass, false);
      s = 5; // set to 5 seconds in case it fails again
    }
  }
}

String WiFiManager::page()
{
  String s = HTTP_HEAD;
  s += HTTP_SCRIPT;
  s += HTTP_STYLE;
  s += HTTP_HEAD_END;

  WiFi.scanNetworks();
  for (int i = 0;  WiFi.SSID(i).length(); ++i)
  {
    DEBUG_PRINT(WiFi.SSID(i));
    DEBUG_PRINT(WiFi.RSSI(i));
    String item = HTTP_ITEM;
    item.replace("{v}", WiFi.SSID(i) );
    s += item;
  }
  
  String form = HTTP_FORM;
  form.replace("$key", _pPass );
  s += form;
  s += HTTP_END;
  
  _timeout = false;
  return s;
}

String WiFiManager::urldecode(const char *src)
{
    String decoded = "";
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a')
                a -= 'a'-'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a'-'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            
            decoded += char(16*a+b);
            src+=3;
        } else if (*src == '+') {
            decoded += ' ';
            *src++;
        } else {
            decoded += *src;
            *src++;
        }
    }
    decoded += '\0';
    
    return decoded;
}

void WiFiManager::drawSpinner(int count, int active) {
  for (int i = 0; i < count; i++) {
    const char *xbm;
    if (active == i) {
       xbm = active_bits;
    } else {
       xbm = inactive_bits;  
    }
    display.drawXbm(64 - (12 * count / 2) + 12 * i,56, 8, 8, xbm);
  }   
}
