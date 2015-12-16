# webserv
A simple http web server written in C

Craig Tateronis
CS410 - Fall 2015

~~~~~FILES IN PROJECT~~~~~
webserv.c 	- source for webserv binary
filecache.c 	- source for file cache structure and methods
filecache.h 	- library file linking webserv.c and filecache.c
README 		- This document
Makefile 	- The makefile for webserv.c and filecache.c
/www/ 		- root directory for web server resources
	index.html		- main page for web server
	my-histogram.cgi 	- generates a frequency plot table for gnuplot
	display-histogram.cgi 	- generate histogram and embedded html document from my-histogram.cgi data
	image.html		- example output of display-histogram.cgi
	binduino.cgi		- executable for sending binary strings to arduino over web browser
	favicon.ico		- address bar icon (clients keep requesting it so I added it)
	/err/			- directory for error code html pages (I felt this was better than hard-coded solution)
	/icons/			- standard apache icon files for directory tree generation
	/nested/		- test html and image files for web server
	/old/			- deprecated Perl based executable for histogram generation

~~~~~BONUS PROJECT (BINDUINO)~~~~~
Unforunately my original project was not implemented because the parts I ordered nearly a month ago are still (at the time of writing) stuck in processing and will apparently not arrive until after the end of the course.

Instead I decided to construct an array of 8 LEDs, and have the Arduino flash bytes of binary supplied by serial input.  To that end I developed an Arduino sketch that listens for input, and a cgi script that allows a client to send binary strings to the arduino when it is connected to the server.  For example, the url localhost:9999/binduino.cgi?110011011 will flash the bytes "11001101" on the LEDs connected to the Arduino.  Will currently ignore any excess bits in the string, future goals are to accept chars and print them in sequence, or use a less terrible hardware display.


~~~~~CHALLENGES/OUTSTANDING ISSUES~~~~~
	CGI files are executed by the server, but client only receives output when server process is killed (or not at all for larger output).  This is almost certainly due to faulty HTTP headers, probably more specifically involving Content-Length.

	Cache structure was functional and operational on single process implementation, but not functional on multi-process due to complexity of memory sharing linked lists in C.  By default changes made to the forked cache are not reflected in the parent, despite attempts to use shared memory segments.  Would probably be functional in a multithreaded implementation, but web server would have required substantial overhaul and I ran out of time.  Webserv accepts a third argument in the form of a cache size (in bytes) or --nocache to disable.

	Pages occasionally require multiple clicks or refreshes before loading on the client.  This is probably either due to multiprocess or HTTP header issues.
