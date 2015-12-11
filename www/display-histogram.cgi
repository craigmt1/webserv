#! /usr/bin/env python
import sys, re, commands
from subprocess import call

#arg example: ./display-histogram.cgi "?name1=a3.html&name2=and&name3=but&name4=so&name5=hello"
#generates  : ./my-histogram.cgi a3.html "and" "but" "so" "he.*lo" | gnuplot -p -e "set term png; set style fill solid 1.0 noborder; set boxwidth 0.5 absolute; plot '-' using 1:2:xtic(3) with boxes title \"Occurences\"" | base64

#first command (my-histogram.cgi) builds table for graph
#second command (gnuplot) generates graph from table
#third command (base64) outputs string of raw png data

#then html string generated, raw image data is injected directly into html

#inspect arguments
if len(sys.argv) != 2:
	exit(-1);

#tokenize url arguments
tokens = re.split("\&* *\w*\=", sys.argv[1])

#build command string
cmd = "./my-histogram.cgi " + tokens[1] + ' "' + '" "'.join(tokens[2:]) + '"'
cmd += " | gnuplot -p -e \"set term png; set style fill solid 1.0 noborder; set boxwidth 0.5 absolute; plot '-' using 1:2:xtic(3) with boxes title \\\"Occurences in " + tokens[1] + "\\\"\" | base64"

#execute 
cmdout = commands.getoutput(cmd)

#build html string
page = "<!DOCTYPE html>\
	\n<html>\
	\n\t<body>\
	\n\t\t<center>\
	\n\t\t\t<font size=\"3\" color=\"red\">\
	\n\t\t\t\t<p>CS410 Webserver</p>\
	\n\t\t\t</font>\
	\n\t\t\t<br></br>"

if len(cmdout) > 1000:
	page += "\n\t\t\t<img src=\"data:image/png;base64," + cmdout + "\" alt=\"histogram\">";

page += "\n\t\t</center>\
	\n\t</body>\
	\n</html>";

#print str(len(page)) + " Bytes"
print "Content-type: text/html"
print page
