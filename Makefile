all: webserv

webserv: webserv.c
	gcc -o webserv webserv.c filecache.c

clean:
	rm -f webserv
