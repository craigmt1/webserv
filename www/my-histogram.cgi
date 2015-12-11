#! /usr/bin/env python
import sys, re

#example usage
#./my-histogram.cgi a3.html "and" "but" "so" "he.*lo" | gnuplot -p -e "set term png; set style fill solid 1.0 noborder; set boxwidth 0.5 absolute; plot '-' using 1:2:xtic(3) with boxes title \"Occurences\"" | base64

#inspect arguments
if len(sys.argv) < 3:
	if len(sys.argv) < 2: exit('-1')
	else: exit('0')
try: fp = open(sys.argv[1], 'r')
except: exit('-2')

#collect regex data
i = 1
for arg in sys.argv[2:]:
	fp.seek(0)
	print("%d %d %s" % (i, len(re.findall(r'' + arg, fp.read())), arg))
	i+=1

fp.close()

