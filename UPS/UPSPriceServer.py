#!/usr/bin/python
#
# UPSPriceServer.py - Interface the Weasure scale to get UPS shipping price quotes
#
# (c) 2005 John W. Peterson.  Permission to use and re-distribute for non-commercial
# use granted, so long as this copyright notice and author credit remain with the program.
# For commercial use, please contact the author at saccade at-sign gmail.com
#
# This implements a server / web based user interface between the Weasure scale
# and UPS's pricing information.  It launches a web browser to access a web server
# on the local host.  The server formats a template web form, substituting in the
# Weasure measurements.  When you fill out the web page form and submit, it sends
# the data to UPS, collects a pricing page, and feeds it back to your browser.  Thus
# the burden of implementing a user interface is make on creating a web form, rather
# than writing code for it all.
#
# For more information on the Weasure scale, go to www.weasure.com
#
# For details on accessing the rate information from the UPS web site, see:
#   "UPS Online Tools Developer Guide: Rates and Service Selection HTML Tool"
# published as part of the UPS Online Tools program


# Grab some generic system tools
import os, sys, time, re, string
# Grab some web page related tools
import urllib, BaseHTTPServer, webbrowser
# Additional package for serial I/O, from http://pyserial.sourceforge.net/
import serial

# Location of the HTML Form Template
# Must be next to where this script is
ScriptPath = os.path.dirname(os.path.realpath(sys.argv[0])) + os.sep
FormTemplatePath = ScriptPath + 'Template.html'
# On some systems, the traditional HTTP port, 80, is not
# available locally.  So we use an alternate one that is.
LocalPortNumber = 8080

# Keys shared by the UPS and weasure pages
WEIGHT_TAG = '23_weight'
LENGTH_TAG = '25_length'
HEIGHT_TAG = '27_height'
WIDTH_TAG  = '26_width'

# This table defines the keys and data flags sent
# to the UPS server.
# See the UPS OnLine Tools Developer's Guide:
#    Rates and Service Selection HTML Tool
# https://www.ups.com/gec/techdocs/pdf/RatesandServiceHTML.pdf
UPSPostData = {
'accept_UPS_license_agreement'  :'yes',
'10_action'         :'4',
#'13_product'        :'GND',	# Not needed if '10_action' is 4
'14_origCountry'    :'US',
'origCity'          :'Menlo Park',
'15_origPostal'     :'94025',
'22_destCountry'    :'US',
'20_destCity'       :'Forest Grove',
'19_destPostal'     :'97116',
'49_residential'    :'1',
'shipDate'          :"%4d-%02d-%02d" % (time.localtime()[0], time.localtime()[1], time.localtime()[2]+2),
'47_rate_chart'     :'Customer Counter',
'48_container'      :'00',
'billToUPS'         :'no',
'24_value'          :'99',
'currency'          :'USD',
'weight_std'        :'lbs.',
'length_std'        :'in.',
# To be supplied by Weasure
WEIGHT_TAG          :5,
LENGTH_TAG          :6,
WIDTH_TAG	        :7,
HEIGHT_TAG          :8
}

# Take the data from the UPSPostData, and send it to the UPS server
# to get a cost estimate page.
def FetchUPSRatePage(browserStream):
	UPS_url = "http://wwwapps.ups.com/ctc/htmlTool"
	UPS_POST = string.join(["%s=%s" % (k, str(UPSPostData[k])) for k in UPSPostData.keys()], '&')
	urlStream = urllib.urlopen(UPS_url, UPS_POST)
	result = urlStream.read()
	urlStream.close()
	# Fool the browser into thinking the page we're serving locally
	# originated at UPS, so images and style sheets work correctly.
	result = '<BASE href="%s">' % UPS_url + result
	browserStream.write( result )

# Query Weasure via the serial port for the
# current set of weight and measurements
def UpdateMeasurements(values):
	# Take this line out if you actually have the Weasure scale
	# connected.  Otherwise it just returns without doing anything
	return

	ser = serial.Serial('Com1:', 19200, timeout=1)
	ser.write('s')		# Get the size
	s = ser.read(10)	# hh,ww,dd\r\n
	(values[LENGTH_TAG], values[WIDTH_TAG], values[HEIGHT_TAG]) = map(int, s[:-2].split(','))
	ser.write('w')		# Get the weight
	s = ser.read(7)		# PP,OO\r\n
	if (s[:3] == 'OFF'):
		values[WEIGHT_TAG] = 0.0
	else:
		(lbs, oz) = map(int, s[:-2].split(','))
		values[WEIGHT_TAG] = lbs + oz/16.0
	ser.close()

# Read in the template Weasure HTLM form, and substitute Weasure
# measurements found in the "values" table with anchor tags in the HTML
def ReplaceHTMLTemplateValues(template, values):
	# Define a pattern to match the template values
	a_tagRE = re.compile('<[Aa]\s*[Nn][Aa][Mm][Ee]="(\w*)">(\d*)</[Aa]>')
	result = ''
	atag_iter = a_tagRE.finditer(template)
	pos = 0
	try:
		# Loop through the matches of the <A NAME=xxx>yyy</A> tags.  When
		# a match is found, splice in the values[xxx] for yyy
		# atag_match.group(1) is xxx, group(2) is yyy
		while (1):
			atag_match = atag_iter.next()
			tagname = atag_match.group(1)
			result = result + template[pos:atag_match.start(2)] + str(values[tagname])
			pos = atag_match.end(2)
	except StopIteration:
		result = result + template[pos:]
	return result

# Implement a web server to display the form and handle responses from it
def LocalHTTPServer():
	formTemplate = file(FormTemplatePath, 'r').read()
	HTTPHeader = 'HTTP/1.1 200 OK\nServer: Weasure\nContent-Type: text/html\n\n'
	global quitFlag
	quitFlag = False
	UpdateMeasurements(UPSPostData)

	# This class defines what happens to the data coming back from
	# the web page.
	class LocalRequestHandler( BaseHTTPServer.BaseHTTPRequestHandler):
		# This hand the "POST" requests from pressing form buttons
		def do_POST(self):
			print self.path
			if (self.path == '/weasure'):
				self.rfile.read(int(self.headers['Content-Length']))
				UpdateMeasurements(UPSPostData)
				self.do_GET()
			else:				
				for i in self.rfile.read(int(self.headers['Content-Length'])).split('&'):
					t = i.split('=')
					UPSPostData[t[0]] = t[1]
				self.wfile.write(HTTPHeader)
				FetchUPSRatePage(self.wfile)

		# This handles regular links
		def do_GET(self):
			global quitFlag
			self.wfile.write(HTTPHeader)
			if (self.path == '/quit'):	# Quit link selected, exit.
				quitFlag = True;
				self.wfile.write("Server exiting, close window...")
			else:
				self.wfile.write(ReplaceHTMLTemplateValues(formTemplate, UPSPostData))

	# Start the web server, using the request handler defined above
	httpd = BaseHTTPServer.HTTPServer(('',LocalPortNumber), LocalRequestHandler)

	# Keep handling requests until the quitFlag is set
	while (not quitFlag):
		httpd.handle_request()
	print "Exiting server"

# Open the browser on our local web server, and start the server
webbrowser.open_new('http://127.0.0.1:%d' % LocalPortNumber)
LocalHTTPServer()

