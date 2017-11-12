# WiFi Car Sentry
Low power car monitor and control system  
  
Revision 2  
  
Using the ESP07, it connects with the home WiFi to report information on timer using DeepSleep.  Remember to remove the red LED on the ESP to reduce current.  This new revision uses a switching regulator for the lowest current possible.  It uses about 25mA on, and 39µA in sleep at 12.5V.  
  
I/O peripherals:  
  OLED (optional for temporary display)  
  DHT22 for temperature and Rh  
  0-17V analog voltage reading  
  2x 1-40V digital input (for sensing if something is on)  
  3x sink inputs (alarm or sensor)  
  3 sink outputs for controlling relays, etc (the bom has 7.6A MOSFETs)  
  
![esp07carsentry](http://www.curioustech.net/images/carsentry.png)
