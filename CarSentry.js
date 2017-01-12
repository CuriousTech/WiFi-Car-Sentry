// Car Sentry stream listener (using CuriousTech PngMagic)
// Displays volts, temp and input status and sends command to sleep
// The "Open Page" button will on the browser to view the page on the device and keep it on the next time it connects

csIP = '192.168.0.112:81' // set this to the IP and port it uses (forced in router)
Url = 'ws://' +csIP + '/ws'

browser = 'C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe'

sleepTime = 60 * 10 // sleep for 10 minutes (sent to CarSentry)
pass = 'password'   // current devicePassword

Pm.Window('CarSentry')

Gdi.Width = 208 // resize drawing area
Gdi.Height = 100

if(!Http.Connected)
	Http.Connect( 'event', Url )  // Start the event stream

var last
var Json
date = new Date()
lastT = (date.valueOf()/1000).toFixed()
event = ''
error = 0
load = false
cnt = 5 // 5 second counter
Pm.SetTimer(1000) // run every second
heartbeat = (new Date()).valueOf()
Draw()

// Handle published events
function OnCall(msg, event, data)
{
	switch(msg)
	{
		case 'HTTPDATA':
			heartbeat = new Date()
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
					Draw()
					break
			}
			break
	}
}

function procLine(data)
{
	if(data.length < 2) return
	data = data.replace(/\n|\r/g, "")
	parts = data.split(';')

	switch(parts[0])
	{
		case 'state':
			LogCS(parts[1])
			if(load) // if Open Page was clicked, open the CarSentry in browser (holds power on)
			{
				Pm.Run(browser, 'http://' + csIP )
				load = false
			}
			if(Json.ct) // != 0 (if 0, wait for next send)
			{
				Http.Send( 'cmd;{"key":"' + pass + '","TO":'+ sleepTime +',"sleep":1}' )
				Http.Close()
			}
			Draw()
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
	if(time - heartbeat > sleepTime * 2 *1000)
		error = 1
	if(--cnt == 0)
	{
		cnt = 5
		if(!Http.Connected)
			Http.Connect( 'event', Url )  // Start the event stream
	}
	Draw()
}

function LogCS( str )
{
	Json = !(/[^,:{}\[\]0-9.\-+Eaeflnr-u \n\r\t]/.test(
		str.replace(/"(\\.|[^"\\])*"/g, ''))) && eval('(' + str + ')')

	Pm.Echo('Volts = ' + Json.v + ' temp = ' + Json.ct)
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

	color = Gdi.Argb(255, 255, 0, 0)
	Gdi.Brush( color )
	Gdi.Text( 'X', Gdi.Width-17, 1 )

	Gdi.Font( 'Arial' , 11, 'Regular')
	Gdi.Brush( Gdi.Argb(255, 255, 255, 255) )

	x = 5
	y = 20
	if(Json == undefined) // no data yet
		return

	date = new Date(Json.t*1000)
	Gdi.Brush( error ? Gdi.Argb(255, 255, 0, 0) : Gdi.Argb(255, 255, 255, 255) )

	Gdi.Text( date.toLocaleTimeString(), Gdi.Width-84, 2 )

	Gdi.Brush( Gdi.Argb(255, 255, 255, 255) )
	Gdi.Font( 'Arial' , 12, 'Regular')

	Gdi.Text('Temp: ' + Json.ct + '°  Rh: ' + Json.rh +'%', x, y)
	y += 18
	Gdi.Text(' Volts: ' + Json.v + ' In: ' + (Json.on ? 'On':'Off') , x, y)

	drawButton(load ? 'Waiting...' :'Open Page', 10, 60, 80, 16)
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
