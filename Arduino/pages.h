const char page1[] PROGMEM =
   "<!DOCTYPE html><html lang=\"en\">\n"
   "<head>\n"
   "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/>\n"
   "<title>WiFi Car Sentry</title>\n"
   "<style type=\"text/css\">\n"
   "table,input{\n"
   "border-radius: 5px;\n"
   "box-shadow: 2px 2px 12px #000000;\n"
   "background-image: -moz-linear-gradient(top, #dfffff, #5050ff);\n"
   "background-image: -ms-linear-gradient(top, #dfffff, #5050ff);\n"
   "background-image: -o-linear-gradient(top, #dfffff, #5050ff);\n"
   "background-image: -webkit-linear-gradient(top, #dfffff, #5050ff);\n"
   "background-image: linear-gradient(top, #dfffff, #5050ff);\n"
   "background-clip: padding-box;\n"
   "}\n"
   "body{width:320px;display:block;margin-left:auto;margin-right:auto;text-align:right;font-family: Arial, Helvetica, sans-serif;}}\n"
   "</style>\n"
   "<script type=\"text/javascript\">\n"
   "a=document.all;oledon=0;o1=0;o2=0;o3=0;gt=0\n"
   "function openSocket(){\n"
   "ws=new WebSocket(\"ws://\"+window.location.host+\"/ws\")\n"
   "ws.onopen=function(evt) { }\n"
   "ws.onclose=function(evt) { alert(\"Connection closed.\"); }\n"
   "ws.onmessage=function(evt) {\n"
   " lines=evt.data.split(';')\n"
   " event=lines[0]\n"
   " data=lines[1]\n"
   " if(event=='state')\n"
   " {\n"
   "  d=JSON.parse(data)\n"
   "  dt=new Date(d.t*1000)\n"
   "  a.time.innerHTML=dt.toLocaleTimeString()\n"
   "  a.ct.innerHTML=d.ct.toFixed(1)+'&degF'\n"
   "  a.on.innerHTML='In12V: '+(d.in12v?\"<font color='red'><b>ON</b></font>\":\"OFF\")\n"
   "  a.in1.innerHTML='In1: '+(d.in1?\"<font color='red'><b>ON</b></font>\":\"OFF\")\n"
   "  a.in2.innerHTML='In2: '+(d.in2?\"<font color='red'><b>ON</b></font>\":\"OFF\")\n"
   "  a.rh.innerHTML=d.rh.toFixed(1)+'%'\n"
   "  a.v.innerHTML=d.v.toFixed(2)+'v'\n"
   "  a.LAT.innerHTML=d.lat\n"
   "  a.LON.innerHTML=d.lon\n"
   " }\n"
   " else if(event=='set')\n"
   " {\n"
   "  d=JSON.parse(data)\n"
   "  oledon=d.o\n"
   "  a.OLED.value=oledon?'ON ':'OFF'\n"
   "  gt=d.gt\n"
   "  a.GT.value=gt?'ON ':'OFF'\n"
   "  gl=d.gl\n"
   "  a.GL.value=gl?'ON ':'OFF'\n"
   "  a.tz.value=d.tz\n"
   "  a.to.value=d.to\n"
   "  o1=d.o1\n"
   "  o2=d.o2\n"
   "  o3=d.o3\n"
   "  a.O1.value=o1?'ON ':'OFF'\n"
   "  a.O2.value=o2?'ON ':'OFF'\n"
   "  a.O3.value=o3?'ON ':'OFF'\n"
   "  ro=d.ro\n"
   "  a.RO.value=d.ro?'ON ':'OFF'\n"
   "  a.domain.value=d.d\n"
   " }\n"
   " else if(event=='alert')\n"
   " {\n"
   "  alert(data)\n"
   " }\n"
   "}\n"
   "}\n"
   "function setVar(varName, value)\n"
   "{\n"
   " ws.send('cmd;{\"key\":\"'+a.myKey.value+'\",\"'+varName+'\":'+value+'}')\n"
   "}\n"
   "function oled(){oledon=!oledon\n"
   "setVar('oled',oledon)\n"
   "a.OLED.value=oledon?'ON ':'OFF'\n"
   "}\n"
   "function setD(){\n"
   "setVar('SD',a.domain.value)\n"
   "}\n"
   "function setTZ(){\n"
   "setVar('TZ',a.tz.value)}\n"
   "function setGT(){\n"
   "gt=!gt\n"
   "setVar('GT',gt)\n"
   "a.GT.value=gt?'ON ':'OFF'\n"
   "}\n"
   "function setGL(){\n"
   "gl=!gl\n"
   "setVar('GL',gl)\n"
   "a.GL.value=gl?'ON ':'OFF'\n"
   "}\n"
   "function setTO(){\n"
   "setVar('TO',a.to.value)}\n"
   "function setO1(){\n"
   "o1=!o1\n"
   "setVar('O1',o1)\n"
   "a.O1.value=o1?'ON ':'OFF'\n"
   "}\n"
   "function setO2(){\n"
   "o2=!o2\n"
   "setVar('O2',o2)\n"
   "a.O2.value=o2?'ON ':'OFF'\n"
   "}\n"
   "function setO3(){\n"
   "o3=!o3\n"
   "setVar('O3',o3)\n"
   "a.O3.value=o3?'ON ':'OFF'\n"
   "}\n"
   "function setRO(){\n"
   "ro=!ro\n"
   "setVar('RO',ro)\n"
   "a.RO.value=ro?'ON ':'OFF'\n"
   "}\n"
   "function pulse(n){\n"
   "setVar('PLS',n)\n"
   "}\n"
   "</script>\n"
   "</head>\n"
   "<body bgcolor=\"silver\" onload=\"{\n"
   "key=localStorage.getItem('key')\n"
   "if(key!=null) document.getElementById('myKey').value=key\n"
   "openSocket()\n"
   "}\">\n"
   "<h3>WiFi Car Sentry</h3>\n"
   "<table align=\"right\">\n"
   "<tr><td align=\"center\" colspan=2><div id=\"time\"></div></td><td><div id=\"v\"></div> </td><td align=\"center\"></td></tr>\n"
   "<tr><td align=\"center\" colspan=2>Env: </td><td><div id=\"ct\"></div></td><td align=\"left\"><div id=\"rh\"></div> </td></tr>\n"
   "<tr><td></td><td><div id=\"in1\"></div></td><td><div id=\"in2\"></div></td><td><div id=\"on\"></div></td></tr>\n"
   "<tr><td colspan=2>Get NTP<input type=\"button\" value=\"ON \" id=\"GT\" onClick=\"{setGT()}\"></td><td colspan=2>Out 1:<input type=\"button\" value=\"OFF\" id=\"O1\" onClick=\"{setO1()}\"><input type=\"button\" value=\"Pulse\" onClick=\"{pulse(1)}\"></td></tr>\n"
   "<tr><td colspan=2>TZ <input id='tz' type=text size=2 value='-5'><input value=\"Set\" type='button' onclick=\"{setTZ()}\"></td><td colspan=2>Out 2:<input type=\"button\" value=\"OFF\" id=\"O2\" onClick=\"{setO2()}\"><input type=\"button\" value=\"Pulse\" onClick=\"{pulse(2)}\"></td></tr>\n"
   "<tr><td colspan=2>Time <input id='to' type=text size=2 value='10'><input value=\"Set\" type='button' onclick=\"{setTO()}\"></td><td colspan=2>Out 3:<input type=\"button\" value=\"OFF\" id=\"O3\" onClick=\"{setO3()}\"><input type=\"button\" value=\"Pulse\" onClick=\"{pulse(3)}\"></td></tr>\n"
   "<tr><td colspan=2>Get LOC<input type=\"button\" value=\"ON \" id=\"GL\" onClick=\"{setGL()}\"></td><td colspan=2><div id=\"LAT\"></div><div id=\"LON\"></div></td></tr>\n"
   "<tr><td colspan=2>Roaming<input type=\"button\" value=\"ON \" id=\"RO\" onClick=\"{setRO()}\"></td><td colspan=2><input id=\"domian\" name=\"domain\" type=text size=50 placeholder=\"127.0.0.1\" style=\"width: 130px\"></td></tr>\n"
   "<tr><td colspan=2>Display <input type=\"button\" value=\"ON \" id=\"OLED\" onClick=\"{oled()}\"></td><td colspan=2><input id=\"myKey\" name=\"key\" type=text size=40 placeholder=\"password\" style=\"width: 90px\"><input type=\"button\" value=\"Save\" onClick=\"{localStorage.setItem('key',key=document.all.myKey.value);setD()}\"></td></tr>\n"
   "</table>\n"
   "</body>\n"
   "</html>\n";

const uint8_t favicon[] PROGMEM = {
  0x1F, 0x8B, 0x08, 0x08, 0x70, 0xC9, 0xE2, 0x59, 0x04, 0x00, 0x66, 0x61, 0x76, 0x69, 0x63, 0x6F, 
  0x6E, 0x2E, 0x69, 0x63, 0x6F, 0x00, 0xD5, 0x94, 0x31, 0x4B, 0xC3, 0x50, 0x14, 0x85, 0x4F, 0x6B, 
  0xC0, 0x52, 0x0A, 0x86, 0x22, 0x9D, 0xA4, 0x74, 0xC8, 0xE0, 0x28, 0x46, 0xC4, 0x41, 0xB0, 0x53, 
  0x7F, 0x87, 0x64, 0x72, 0x14, 0x71, 0xD7, 0xB5, 0x38, 0x38, 0xF9, 0x03, 0xFC, 0x05, 0x1D, 0xB3, 
  0x0A, 0x9D, 0x9D, 0xA4, 0x74, 0x15, 0x44, 0xC4, 0x4D, 0x07, 0x07, 0x89, 0xFA, 0x3C, 0x97, 0x9C, 
  0xE8, 0x1B, 0xDA, 0x92, 0x16, 0x3A, 0xF4, 0x86, 0x8F, 0x77, 0x73, 0xEF, 0x39, 0xEF, 0xBD, 0xBC, 
  0x90, 0x00, 0x15, 0x5E, 0x61, 0x68, 0x63, 0x07, 0x27, 0x01, 0xD0, 0x02, 0xB0, 0x4D, 0x58, 0x62, 
  0x25, 0xAF, 0x5B, 0x74, 0x03, 0xAC, 0x54, 0xC4, 0x71, 0xDC, 0x35, 0xB0, 0x40, 0xD0, 0xD7, 0x24, 
  0x99, 0x68, 0x62, 0xFE, 0xA8, 0xD2, 0x77, 0x6B, 0x58, 0x8E, 0x92, 0x41, 0xFD, 0x21, 0x79, 0x22, 
  0x89, 0x7C, 0x55, 0xCB, 0xC9, 0xB3, 0xF5, 0x4A, 0xF8, 0xF7, 0xC9, 0x27, 0x71, 0xE4, 0x55, 0x38, 
  0xD5, 0x0E, 0x66, 0xF8, 0x22, 0x72, 0x43, 0xDA, 0x64, 0x8F, 0xA4, 0xE4, 0x43, 0xA4, 0xAA, 0xB5, 
  0xA5, 0x89, 0x26, 0xF8, 0x13, 0x6F, 0xCD, 0x63, 0x96, 0x6A, 0x5E, 0xBB, 0x66, 0x35, 0x6F, 0x2F, 
  0x89, 0xE7, 0xAB, 0x93, 0x1E, 0xD3, 0x80, 0x63, 0x9F, 0x7C, 0x9B, 0x46, 0xEB, 0xDE, 0x1B, 0xCA, 
  0x9D, 0x7A, 0x7D, 0x69, 0x7B, 0xF2, 0x9E, 0xAB, 0x37, 0x20, 0x21, 0xD9, 0xB5, 0x33, 0x2F, 0xD6, 
  0x2A, 0xF6, 0xA4, 0xDA, 0x8E, 0x34, 0x03, 0xAB, 0xCB, 0xBB, 0x45, 0x46, 0xBA, 0x7F, 0x21, 0xA7, 
  0x64, 0x53, 0x7B, 0x6B, 0x18, 0xCA, 0x5B, 0xE4, 0xCC, 0x9B, 0xF7, 0xC1, 0xBC, 0x85, 0x4E, 0xE7, 
  0x92, 0x15, 0xFB, 0xD4, 0x9C, 0xA9, 0x18, 0x79, 0xCF, 0x95, 0x49, 0xDB, 0x98, 0xF2, 0x0E, 0xAE, 
  0xC8, 0xF8, 0x4F, 0xFF, 0x3F, 0xDF, 0x58, 0xBD, 0x08, 0x25, 0x42, 0x67, 0xD3, 0x11, 0x75, 0x2C, 
  0x29, 0x9C, 0xCB, 0xF9, 0xB9, 0x00, 0xBE, 0x8E, 0xF2, 0xF1, 0xFD, 0x1A, 0x78, 0xDB, 0x00, 0xEE, 
  0xD6, 0x80, 0xE1, 0x90, 0xFF, 0x90, 0x40, 0x1F, 0x04, 0xBF, 0xC4, 0xCB, 0x0A, 0xF0, 0xB8, 0x6E, 
  0xDA, 0xDC, 0xF7, 0x0B, 0xE9, 0xA4, 0xB1, 0xC3, 0x7E, 0x04, 0x00, 0x00, 
};
