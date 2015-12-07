#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>


//http://blog.manula.org/2011/05/writing-simple-web-server-in-c.html
//http://shoe.bocks.com/net/
//http://www.tldp.org/LDP/LGNET/91/misc/tranter/server.c.txt
//http://stackoverflow.com/questions/10072989/simple-http-server-in-c-multiple-process-not-work-properly

int port_no; //port number for webserv

char *req_buf; //buffer for messages from client socket
int req_buf_size; //size of buffer

int serv_sockfd; //server socket file descriptor
size_t serv_sock_size; //size of server socket file descriptor
int client_sockfd; //client socket file descriptor

struct sockaddr_in addr; //server socket address structure
socklen_t addrlen; //address structure size

void exithandler();
void send_html(int client_sockfd, int req_fd, char *filename);

int main(int argc, char **argv){
    //check arguments
    if (argc < 2 || argc > 2) {
        printf("Must specify port number as argument!\n");
        exit(0);
    }
    //cast port number to integer and check if valid
    port_no = atoi(argv[1]); //get port# from argv
    if (port_no < 5000 || port_no > 65536) {
    	printf("Invalid port number, must be integer between 5000 and 65536\n");
    	exit(0);
    }

    //create server socket http://linux.die.net/man/7/socket
    serv_sock_size = sizeof(addr);
    req_buf_size = 1024; //allocate request message buffer
    req_buf = malloc(req_buf_size);

    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_port = htons(port_no);

    //bind socket to address http://linux.die.net/man/2/bind
    if (bind(serv_sockfd, (struct sockaddr*) &addr, serv_sock_size) < 0) {
        perror("Failed to bind socket");
        exit(EXIT_FAILURE);
    }
    //listen for exit signal (ctrl-c)
    signal(SIGINT, exithandler);
    signal(SIGQUIT, exithandler);
    printf("Successfully bound socket %d to port %d.  Press (Ctrl-C) to exit.\nListening...\n", serv_sockfd, port_no);

    //begin listen loop
    while(1) {
        //listen for incoming connections to serv_sockfd http://linux.die.net/man/2/listen
        if (listen(serv_sockfd, 10) < 0) {
            perror("Server Listen Failure");
            exit(EXIT_FAILURE);
        }
        //accept incoming connections from client_sockfd http://linux.die.net/man/2/accept
        if ((client_sockfd = accept(serv_sockfd, (struct sockaddr *) &addr, &addrlen)) < 0) {
            perror("Server Accept Failure");
            exit(EXIT_FAILURE);
        }

        //load messages from client_sockfd into request buffer http://linux.die.net/man/2/recv
        recv(client_sockfd, req_buf, req_buf_size, 0); //not really sure what to do with flag arg
        printf("Request from client socket %d\n%s\n", client_sockfd, req_buf); //output request message to console
        

        //tokenize request buffer (first token is request type)
        char * req_tokens;
        req_tokens = strtok(req_buf, " ");
        //printf("REQUEST TYPE: %s\n", req_tokens);
        int isGet = strcmp(req_tokens, "GET") == 0;
        
        //next token (requested page) copy to another string
        req_tokens = strtok(NULL, " ");
        char *requestedPage = malloc(strlen(req_tokens) + 1);
        strcpy(requestedPage + 1, req_tokens);
        requestedPage[0] = '.';
        printf("PAGE REQUESTED: %s\n", requestedPage);

		if (isGet) {
	        //open file to send to client http://linux.die.net/man/2/open
	        int req_fd;
	        if (strcmp(requestedPage, "./") == 0) req_fd = open("./index.html", O_RDONLY);
	        else req_fd = open(requestedPage, O_RDONLY); //file descriptor for requested page

	        if (req_fd < 0) printf("Unable to fetch file: %s\n", requestedPage);
	        else {
	        	send_html(client_sockfd, req_fd, requestedPage);
	        }
	        close(req_fd); //close file
        }

        close(client_sockfd); //close client connection
    }
    exithandler();
    return 0;
}

void send_html(int client_sockfd, int req_fd, char *filename){
    //get filesize
    struct stat filelen;
    fstat(req_fd, &filelen);

    //tell browser to render html using write http://linux.die.net/man/2/write
    write(client_sockfd, "HTTP/1.1 200 OK\n", 16);

    //content length string (this might not even be necessary?)
    char filesizestr[32];
    sprintf(filesizestr, "%d", (int) filelen.st_size);

    write(client_sockfd, "Content-length: ", 16);
    write(client_sockfd, filesizestr, strlen(filesizestr));
    write(client_sockfd, "\n", 1);

    write(client_sockfd, "Content-Type: text/html\n\n", 25);

    //attempt to send file to client socket
    if (sendfile(client_sockfd, req_fd, 0, filelen.st_size) < 0) {
        printf("Server file send error\nclientfd: %d\tfilename: %s\tfilefd: %d\tfilesize: %d\n", client_sockfd, filename, req_fd, (int) filelen.st_size);
        perror("sendfile");
    }  
}

void exithandler(){
    //gracefully close socket file descriptor
    printf("\nClosing socket %d on port %d...", serv_sockfd, port_no);
    if (close(serv_sockfd) < 0){
        printf(" Failure!\n");
    }
    else printf(" Success!\n");
    free(req_buf);
    exit(0);
}
