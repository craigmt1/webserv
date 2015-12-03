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

int port_no; //port number for webserv

char *req_buf; //buffer for messages from client socket
int req_buf_size; //size of buffer

int serv_sockfd; //server socket file descriptor
size_t serv_sock_size; //size of server socket file descriptor
int client_sockfd; //client socket file descriptor

struct sockaddr_in addr; //server socket address structure
socklen_t addrlen; //address structure size

void exithandler();

int main(int argc, char **argv){
    //check arguments
    if (argc < 2 || argc > 2) {
        printf("Must specify port number as argument!\n");
        exit(0);
    }

    //init variables and create socket http://linux.die.net/man/7/socket
    port_no = atoi(argv[1]); //get port# from argv
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

    //here we go!
    int alive = 1;
    while(alive) {
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
        
        //open file to send to client http://linux.die.net/man/2/open
        char *filename = "./index.html";
        int req_fd = open(filename, O_RDONLY); //file descriptor for requested page
        if (req_fd < 0) printf("Unable to fetch file: %s\n", filename);
        else {
            //get filesize
            struct stat filelen;
            fstat(req_fd, &filelen);

            //tell browser to render html using write http://linux.die.net/man/2/write
            write(client_sockfd, "HTTP/1.1 200 OK\n", 16);

            //content length string (this might not even be necessary?)
            char filesizestr[32];
            sprintf(filesizestr, "%d", filelen.st_size);

            write(client_sockfd, "Content-length: ", 16);
            write(client_sockfd, filesizestr, strlen(filesizestr));
            write(client_sockfd, "\n", 1);

            write(client_sockfd, "Content-Type: text/html\n\n", 25);

            //attempt to send file to client socket
            if (sendfile(client_sockfd, req_fd, 0, filelen.st_size) < 0) {
                printf("Server file send error\nclientfd: %d\tfilename: %s\tfilefd: %d\tfilesize: %d\n", client_sockfd, filename, req_fd, filelen.st_size);
                perror("sendfile");
            }  
        }
        close(req_fd); //close file
        close(client_sockfd); //close client connection
    }
    exithandler();
    return 0;
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
