# WiFi Car Sentry
Low power car monitor and control system  
  
This is an incomplete project currently.  
  
Using the ESP07, it connects with the home WiFi to report information on timer using DeepSleep.  Remember to remove the red LED on the ESP to reduce current.  It uses about 700µA with the power LED and 100µA without.  
  
I/O peripherals:  
  OLED (for temporary display)  
  0-17V analog voltage reading  
  DHT22 for temperature and Rh  
  1-30V digital input (for sensing if something is on)  
  1 sink input (alarm or sensor)  
  2 sink outputs for controlling relays, etc  
  Serial may be used later  
  
![esp07carsentry](http://www.curioustech.net/images/carsentry.jpg) 
