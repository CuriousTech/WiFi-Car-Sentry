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
   "a=document.all;oledon=0;o1=0;o2=0\n"
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
   "  a.on.innerHTML=(d.in12v?\"<font color='red'><b>ON</b></font>\":\"OFF\")\n"
   "  a.in1.innerHTML=(d.in1?\"<font color='red'><b>ON</b></font>\":\"OFF\")\n"
   "  a.in2.innerHTML=(d.in2?\"<font color='red'><b>ON</b></font>\":\"OFF\")\n"
   "  a.rh.innerHTML=d.rh.toFixed(1)+'%'\n"
   "  a.v.innerHTML=d.v.toFixed(2)+'v'\n"
   " }\n"
   " else if(event=='set')\n"
   " {\n"
   "  d=JSON.parse(data)\n"
   "  oledon=d.o\n"
   "  a.OLED.value=oledon?'ON ':'OFF'\n"
   "  a.tz.value=d.tz\n"
   "  a.to.value=d.to\n"
   "  o1=d.o1\n"
   "  o2=d.o2\n"
   "  o3=d.o3\n"
   "  a.O1.value=o1?'ON ':'OFF'\n"
   "  a.O2.value=o2?'ON ':'OFF'\n"
   "  a.O3.value=o3?'ON ':'OFF'\n"
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
   "function setTZ(){\n"
   "setVar('TZ',a.tz.value)}\n"
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
   "<tr><td></td><td>In 1:<div id=\"in1\"></div></td><td>In 2:<div id=\"in2\"></div></td><td>In12V:<div id=\"on\"></div></td></tr>\n"
   "<tr><td colspan=2>TZ <input id='tz' type=text size=2 value='-5'><input value=\"Set\" type='button' onclick=\"{setTZ()}\"></td><td colspan=2>Out 1:<input type=\"button\" value=\"OFF\" id=\"O1\" onClick=\"{setO1()}\"><input type=\"button\" value=\"Pulse\" onClick=\"{pulse(1)}\"></td></tr>\n"
   "<tr><td colspan=2>Time <input id='to' type=text size=2 value='10'><input value=\"Set\" type='button' onclick=\"{setTO()}\"></td><td colspan=2>Out 2:<input type=\"button\" value=\"OFF\" id=\"O2\" onClick=\"{setO2()}\"><input type=\"button\" value=\"Pulse\" onClick=\"{pulse(2)}\"></td></tr>\n"
   "<tr><td colspan=2>Display:<input type=\"button\" value=\"ON \" id=\"OLED\" onClick=\"{oled()}\"></td><td colspan=2>Out 3:<input type=\"button\" value=\"OFF\" id=\"O3\" onClick=\"{setO3()}\"><input type=\"button\" value=\"Pulse\" onClick=\"{pulse(3)}\"></td></tr>\n"
   "<tr><td colspan=2></td><td colspan=2><input id=\"myKey\" name=\"key\" type=text size=40 placeholder=\"password\" style=\"width: 90px\"><input type=\"button\" value=\"Save\" onClick=\"{localStorage.setItem('key', key = document.all.myKey.value)}\"></td></tr>\n"
   "</table>\n"
   "</body>\n"
   "</html>\n";
