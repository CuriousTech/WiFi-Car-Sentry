// PngMagic minimal server script (server will be active on port 83)
// Waits for CarSentry to send a request, then pokes CarSentry.js to time the next WebSocket open at the optimum time

localPort = 83

Http.Close()
Http.Server(localPort)

function OnCall( msg, cl, path )
{
	switch(msg)
	{
		case 'HTTPREQ':
Pm.Echo('Remote IP: ' + Http.IP + ' URL: ' + path)
			if(path == '/')
			{
				for(i = 0; i < Http.ParamCount; i++)
				{
					Pm.Echo('Param ' + i + ': ' + Http.Params(i, 0) + ' = ' + Http.Params(i, 1) )
				}
				sendPage('Server Stuff')
			}
			else if(path == '/car')
				Pm.CarSentry('CHECK', Http.IP, Http.Params(0,1), Http.Params(1,1) )
			else Http.Send( "HTTP/1.1 404 Not Found\r\n\r\n" )
			break
		case 'HTTPDATA':
			Pm.Echo('Data from ' + cl + ' : ' + path)
			break
		case 'HTTPSENT':
			Pm.Echo('Sent ' + cl + ' ' + path)
			break
		case 'HTTPSTATUS':
			Pm.Echo('HTTP Server Status ' + cl)
			break
	}
}

function sendPage(title)
{
	s = 'HTTP/1.1 200 OK\r\n\r\n'
	s += '<!DOCTYPE html><html lang="en"><head><meta name="viewport" content="width=device-width, initial-scale=1"/><title>' +
	 title + '</title>'
	s += "<style>div,input {margin-bottom: 5px;}body{width:250px;display:block;margin-left:auto;margin-right:auto;text-align:right;}</style>"

	s += '<body>'
	s += 'Logged IP: ' + Http.IP + '<br>'
	s += '</body></html>'
	Http.Send(s)
}
