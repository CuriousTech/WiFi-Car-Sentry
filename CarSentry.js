// Car Sentry stream listener (using CuriousTech PngMagic)
// Displays volts, temp and input status and sends command to sleep

csIP = '192.168.0.110:81' // set this to the IP and port it uses (forced in router)
Url = 'ws://' +csIP + '/ws'

browser = 'C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe'

pass = 'password'   // current devicePassword

Pm.Window('CarSentry')

Gdi.Width = 230 // resize drawing area (30 hours at 10mins, 63 at 20mins)
Gdi.Height = 120

var last
var Json
var tarray
var rharray
var varray
var timeArray

if( typeof(varray) != 'object')
	varray = []
if( typeof(tarray) != 'object')
	tarray = []
if( typeof(rharray) != 'object')
	rharray = []
if( typeof(timeArray) != 'object')
	timeArray = []

if(!Http.Connected)
	Http.Connect( 'event', Url )  // Start the event stream
date = new Date()
lastDate = date
event = ''
error = 0
load = false
margin = 28
cnt = 5 // 5 second counter
Pm.SetTimer(1000) // redraw and delay for open
heartbeat = date.valueOf()
Draw()

// Handle published events
function OnCall(msg, event, data, a2)
{
	switch(msg)
	{
		case 'HTTPDATA':
			heartbeat = (new Date()).valueOf()
			error = 0
			procLine(data)
			break
		case 'HTTPSTATUS':
			Pm.Echo('CS status ' + event)
			break
		case 'HTTPCLOSE':
//			Pm.Echo('CS closed')
			break
		case 'CHECK':
			cnt = 2 // called by server.  It takes 2 seconds from powerup to get all the readings
			break
		case 'BUTTON':
			switch(event)
			{
				case 0: // Open Page
					load = true
					break
				case 1:
					if(Reg.sleepTime <71) Reg.sleepTime ++
					break
				case 2:
					if(Reg.sleepTime) Reg.sleepTime --
					break
			}
			Draw()
			break
	}
}

function procLine(data)
{
	if(data.length < 2) return
	data = data.replace(/\n|\r/g, "")
	parts = data.split(';')
	lastDate = new Date()

	switch(parts[0])
	{
		case 'state':
			LogCS(parts[1])
			if(load) // if Open Page was clicked, open the CarSentry in browser (holds power on)
			{
				Pm.Run(browser, 'http://' + csIP )
				load = false
			}
			else if(Json.ct && Json.v > 0.1) // (if 0, wait for next send)
			{
				Http.Send( 'cmd;{"key":"' + pass + '","TO":'+ Reg.sleepTime +',"sleep":1}' )
				Http.Close()
			}
			Draw()
			break
		case 'set':
			Pm.Echo('set ' + data)
			break
		case 'print':
			Pm.Echo( 'CS Print: ' + parts[1])
			break
		case 'alert':
			Pm.Echo( 'CS Alert: ' + parts[1])
			Pm.PlaySound('C:\\AndroidShare\\Media\\Audio\\Notifications\\Krypton.wav')
			break
	}
}

function OnTimer()
{
	time = (new Date()).valueOf()
	if(time - heartbeat > Reg.sleepTime * 60 * 1000)
		error = 1
	if(cnt > 0)
		if(--cnt == 0)
		{
			if(!Http.Connected)
			{
				Http.Connect( 'event', Url )  // Start the event stream
			}
		}
	Draw()
}

function LogCS( str )
{
	Json = !(/[^,:{}\[\]0-9.\-+Eaeflnr-u \n\r\t]/.test(
		str.replace(/"(\\.|[^"\\])*"/g, ''))) && eval('(' + str + ')')

	Pm.Echo(str)
	if( Json.ct == 0) return

	varray.push( Json.v )
	tarray.push( Json.ct )
	rharray.push( Json.rh )
	date = new Date()
	timeArray.push( (date.valueOf() / 1000).toFixed() )

	if(varray.length > Gdi.Width - margin*2 )
	{
		varray.shift()
		tarray.shift()
		rharray.shift()
		timeArray.shift()
	}
}

function Draw()
{
	Gdi.Clear(0) // transaprent

	// window border
	Gdi.Brush( Gdi.Argb( 160, 0, 0, 0) )
	Gdi.FillRectangle(0, 0, Gdi.Width-1, Gdi.Height-1)
	Gdi.Pen( Gdi.Argb(255, 0, 0, 255), 1 )
	Gdi.Rectangle(0, 0, Gdi.Width-1, Gdi.Height-1)

	// Title
	Gdi.Font( 'Courier New', 15, 'BoldItalic')
	Gdi.Brush( Gdi.Argb(255, 255, 230, 25) )
	Gdi.Text( 'Car Sentry', 5, 1 )

	Gdi.Brush( Gdi.Argb(255, 255, 0, 0) )
	Gdi.Text( 'X', Gdi.Width-17, 1 )

	Gdi.Font( 'Arial' , 11, 'Regular')
	Gdi.Brush( Gdi.Argb(255, 255, 255, 255) )

	x = 5
	y = 20
	if(Json != undefined) // no data yet
	{
		Gdi.Brush( error ? Gdi.Argb(255, 255, 0, 0) : Gdi.Argb(255, 255, 255, 255) )

		Gdi.Text( lastDate.toLocaleTimeString(), Gdi.Width-84, 2 )

		Gdi.Brush( Gdi.Argb(255, 255, 255, 255) )
		Gdi.Font( 'Arial' , 12, 'Regular')

		Gdi.Text( Json.ct + '°   ' + Json.rh +'%  ' + Json.v + 'V' , x, y)
		y += 15
		x = 2
		Gdi.Text( ' In:', x, y+1)
		x += 20
		Gdi.Text( Json.lat, x+ 110, y-18)
		Gdi.Text( Json.lon, x+ 110, y-6)
		Gdi.Font( 'Courier New', 15, 'BoldItalic')


		Gdi.Brush( Json.in12v ? Gdi.Argb(255, 255, 0, 0) : Gdi.Argb(255, 255, 255, 255) )
		Gdi.Text( (Json.in12v ? 'On':'Off') , x, y)
		x += 36
		Gdi.Brush( Json.in12 ? Gdi.Argb(255, 255, 0, 0) : Gdi.Argb(255, 255, 255, 255) )
		Gdi.Text( (Json.in1 ? 'On':'Off') , x, y)
		x += 36
		Gdi.Brush( Json.in2 ? Gdi.Argb(255, 255, 0, 0) : Gdi.Argb(255, 255, 255, 255) )
		Gdi.Text( (Json.in2 ? 'On':'Off') , x, y)
	}

	Gdi.Font( 'Arial' , 12, 'Regular')
	y = Gdi.Height - 22
	drawButton(load ? 'Waiting...' :'Open Page', 10, y, 80, 16)

	x = Gdi.Width - 90
	Gdi.Text( Reg.sleepTime + ' mins' , x, y)
	drawButton('+', x+=46, y, 20, 16)
	drawButton('-', x+=20, y, 20, 16)

	x = margin
	y = Gdi.Height - 26
	h = 40
	Gdi.Pen( Gdi.Argb(255, 170, 170, 255),1 ) // border
	Gdi.Rectangle(x-1, y-h-1, Gdi.Width - x - margin + 2, h+1)

	daySep(x, y)
	drawArray(rharray, x, y, 20, 20, Gdi.Argb(255, 0, 255, 255),0 )
	drawArray(tarray, x, y, 56, 56, Gdi.Argb(255, 0, 0, 255), 1 ) // temp
	drawArray(varray, x, y, 12, 12, Gdi.Argb(255, 255, 50, 50), 2 ) // volts
}

function drawArray(arr, x, y, low, high, c, t)
{
	Gdi.Pen( c,1 )
	Gdi.Brush( c )

	h = 40
	for(i = 0; i < arr.length; i++)
	{
		if(arr[i] > high) high = arr[i]
		if(arr[i] < low) low = arr[i]
	}
	switch(t)
	{
		case 2: // volts
			Gdi.Text(low.toFixed(1) , 0, y-10)
			Gdi.Text(high.toFixed(1), 0, y-h-5)
			break
		case 1: // temp
			Gdi.Text(low, Gdi.Width-margin, y-10)
			Gdi.Text(high, Gdi.Width-margin, y-h-5)
			break
	}
	r = high-low
	n = y - ( (arr[0]-low) * h / r ).toFixed(1)
	for(i = 1; i < arr.length; i++)
	{
		n1 = y - ((arr[i]-low) * h / r).toFixed(1)
		Gdi.Line(x + i, n, x + i + 1, n1 )
		n = n1
	}
}

function daySep( x, y)
{
	h = 40
	date = new Date(timeArray[0] * 1000)
	for(i = 1; i < timeArray.length; i++)
	{
		date2 = new Date(timeArray[i] * 1000)

		if(date.getHours() != date2.getHours() && !(date2.getHours() &1))
		{
			c = Gdi.Argb(20, 255, 255, 255 )
			if( date2.getHours() == 0) c = Gdi.Argb(255, 255, 255, 255 )
			if( date2.getHours() == 12) c = Gdi.Argb(180, 255, 255, 255 )
			Gdi.Pen(c)
			Gdi.Line(x + i, y-40, x + i, y )
		}
		date = date2
		date2 = null
	}
	date = null
}

function drawButton(text, x, y, w, h)
{
	Gdi.GradientBrush( 0,y, 22, 24, Gdi.Argb(200, 200, 200, 255), Gdi.Argb(200, 60, 60, 255 ), 90)
	Gdi.FillRectangle( x, y, w-2, h, 3)
	ShadowText( text, x+(w/2), y, Gdi.Argb(255, 255, 255, 255) )
	Pm.Button(x, y, w, h)
}

function ShadowText(str, x, y, clr)
{
	Gdi.Brush( Gdi.Argb(255, 0, 0, 0) )
	Gdi.Text( str, x+1, y+1, 'Center')
	Gdi.Brush( clr )
	Gdi.Text( str, x, y, 'Center')
}
