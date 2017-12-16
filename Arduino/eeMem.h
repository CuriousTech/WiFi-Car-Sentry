#ifndef EEMEM_H
#define EEMEM_H

#include <Arduino.h>

struct eeSet // EEPROM backed data
{
  uint16_t size;          // if size changes, use defauls
  uint16_t sum;           // if sum is diiferent from memory struct, write
  char     szSSID[32];
  char     szSSIDPassword[64];
  int8_t  tz;            // Timezone offset from your global server
  bool    bEnableOLED;
  uint16_t hostPort;
  uint32_t hostIP;
  uint32_t time_off;
  uint8_t  get_time;
  uint8_t  get_loc;
  uint8_t  roaming;
  uint8_t  locRoam;
  char     szLat[24];
  char     szLon[24];
  char     szDomain[64];
  uint32_t res[16];
};

extern eeSet ee;

class eeMem
{
public:
  eeMem();
  void update(void);
private:
  uint16_t Fletcher16( uint8_t* data, int count);
};

extern eeMem eemem;

#endif // EEMEM_H
