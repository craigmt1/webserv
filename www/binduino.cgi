#! /usr/bin/env python
import sys, re, commands, glob
from subprocess import call

#arg example: ./binduino.cgi ?110011011

#inspect arguments
#if len(sys.argv) != 2:
#	exit(-1);

#tokenize url arguments
#tokens = re.split("\&* *\w*\=", sys.argv[1])

print "Content-type: text/plain"

#parse arguments
try:
	bstring = sys.argv[1][1:]
	bstring = bstring.ljust(8, '0')

except: bstring = "00000000"
print "Got string %s" % bstring

#find serial devices, print to all of them just to be sure
devs = glob.glob('/dev/ttyACM*')
if len(devs): print "Found serial device(s): %s" % ', '.join(devs) 
else:
	print("No Devices Found!")
	exit()

for dev in devs:
	cmd = "echo \"%s\" > %s" % (bstring, dev)
	commands.getstatusoutput(cmd)
	print "Trying command: %s" % cmd

#print bstring, type(bstring)