#!/usr/bin/perl -w
# my-histogram.cgi -- generates an html page for a histogram
print "Content-type: text/html\n\n";

#$content = $content . "This is a Perl test!\n";
#system('echo "hi!"');

print "<!DOCTYPE html>
<html>
\t<body>
\t\t<center>
\t\t\t<font size=\"3\" color=\"red\">
\t\t\t\t<p>CS410 Webserver</p>
\t\t\t</font>
\t\t\t<br></br>
\t\t\t<img src=\"data:image/png;base64,";

#generate-histogram.cgi?name1=a3.html&name2=and&name2=but&name2=so&name2=hello

$arg = "";

$num_args = $#ARGV + 1;
print "\nARGUMENTS:$num_args";
if ($num_args == 1) {
	use URI;
	my $url = URI->new($ARGV[0]);
	print "\nURL:$url";

	@words = $url->query_form( );
	for ($i=0; $i < @words; $i++) {
		print "$i {$words[$i]}\n";
	}


	print "\n";
	#$arg = './my-histogram.cgi a3.html "and" "but" "so" "he.*lo" | gnuplot -p -e "set term png; set style fill solid 1.0 noborder; set boxwidth 0.5 absolute; plot '-' using 1:2:xtic(3) with boxes title \"Occurences\"" | base64';
}

unless (system($arg)){
	print "\" alt=\"histogram\">";
} else {
	print "\" alt=\"nothing found!\">";
}

print "\t\t</center>
\t</body>
</html>
";

