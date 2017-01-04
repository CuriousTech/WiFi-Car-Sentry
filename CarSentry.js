// Car Sentry stream listener (using CuriousTech PngMagic)
// Displays volts, temp and input status and sends command to sleep

Url = 'ws://192.168.0.112/ws'  // set this to the IP it uses
sleepTime = 60 * 10 // sleep for 10 minutes
pass = 'password'   // current devicePassword

Pm.Window('CarSentry')

Gdi.Width = 208 // resize drawing area
Gdi.Height = 100

if(!Http.Connected)
	Http.Connect( 'event', Url )  // Start the event stream

var last
date = new Date()
lastT = (date.valueOf()/1000).toFixed()
event = ''

Pm.SetTimer(5*1000) // check every 5 seconds
heartbeat = (new Date()).valueOf()
Draw()

// Handle published events
function OnCall(msg, event, data)
{
	switch(msg)
	{
		case 'HTTPDATA':
			heartbeat = new Date()
Pm.Echo('CS data ' + data)
			procLine(data)
			break
		case 'HTTPSTATUS':
			Pm.Echo('CS status ' + event)
			break;
		case 'HTTPCLOSE':
//			Pm.Echo('CS closed')
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
			Http.Send( 'cmd;{"key":"' + pass + '","TO":'+ sleepTime +',"sleep":1}' )
			Http.Close()
			Draw()
			break
		case 'print':
			Pm.Echo( 'CS Print: ' + parts[1])
			break
		case 'alert':
			Pm.Echo( 'CS Alert: ' + parts[1])
			Pm.PlaySound('C:\\AndroidShare\\Media\\Audio\\Notifications\\Krypton.wav')
			break
		case 'hack':
			hackJson = !(/[^,:{}\[\]0-9.\-+Eaeflnr-u \n\r\t]/.test(
				parts[1].replace(/"(\\.|[^"\\])*"/g, ''))) && eval('(' + parts[1] + ')')
			Pm.Echo('CS Hack: ' + hackJson.ip + ' ' + hackJson.pass)
			break
	}
}

function OnTimer()
{
	time = (new Date()).valueOf()
	if(time - heartbeat > sleepTime * 2 *1000)
	{
		Pm.Echo('CS not waking')
	}

	if(!Http.Connected)
		Http.Connect( 'event', Url )  // Start the event stream
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
	y = 22
	if(Json == undefined) // no data yet
		return

	date = new Date(Json.t*1000)
	Gdi.Text( date.toLocaleTimeString(), Gdi.Width-84, 2 )
	Gdi.Font( 'Arial' , 13, 'Regular')

	Gdi.Text('Temp: ' + Json.ct + '°  Rh: ' + Json.rh +'%', x, y)
	y += 20
	Gdi.Text(' Volts: ' + Json.v + ' In: ' + (Json.on ? 'On':'Off') , x, y)
}
