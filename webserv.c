#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

#include <sys/socket.h>
#include <netinet/in.h>


//http://blog.manula.org/2011/05/writing-simple-web-server-in-c.html
//http://shoe.bocks.com/net/
//http://www.tldp.org/LDP/LGNET/91/misc/tranter/server.c.txt
//http://stackoverflow.com/questions/10072989/simple-http-server-in-c-multiple-process-not-work-properly

int port_no; //port number for webserv

int serv_sockfd; //server socket file descriptor
size_t serv_sock_size; //size of server socket file descriptor

struct sockaddr_in addr; //server socket address structure
socklen_t addrlen; //address structure size

void exithandler();
void client_request(int client_sockfd);
void write_dirList(int client_sockfd, int req_fd, char *requestedPage);

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
        int client_sockfd; //client socket file descriptor
        
        //listen for incoming connections to serv_sockfd http://linux.die.net/man/2/listen
        if (listen(serv_sockfd, 10) < 0) {
            perror("Server Listen Failure");
            exit(EXIT_FAILURE);
        }
        //accept incoming connections from client_sockfd http://linux.die.net/man/2/accept
        if ((client_sockfd = accept(serv_sockfd, (struct sockaddr *) &addr, &addrlen)) < 0) perror("Server Accept Failure");

        else {
	    //fork each client request
	    if (fork() == 0){
		client_request(client_sockfd);
		close(client_sockfd);
		exit(0);
	    }
        }
    }
    exithandler();
    return 0;
}

//handle specific request from client socket
void client_request(int client_sockfd){
    //initialize request buffer
    char *req_buf; //buffer for messages from client socket
    int req_buf_size; //size of buffer
    req_buf_size = 1024; //allocate request message buffer
    req_buf = malloc(req_buf_size);

    chdir("./www/"); //change working directory to website directory
    int req_fd; //file descriptor for requested page

    //load messages from client_sockfd into request buffer http://linux.die.net/man/2/recv
    recv(client_sockfd, req_buf, req_buf_size, 0); //not really sure what to do with flag arg
    printf("Request from client socket %d\n%s", client_sockfd, req_buf); //output request message to console
    
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


    //create file descriptor (default: no page specified, return index.html)
    int isIndex;
    if (strcmp(requestedPage, "./") == 0) {
        req_fd = open("./index.html", O_RDONLY);
        isIndex = 1;
    }
    else {
        req_fd = open(requestedPage, O_RDONLY);
        isIndex = 0;
    }

    //404 file not found
    int is404 = 0;
    if (req_fd < 0) {
        write(client_sockfd, "HTTP/1.0 404 Not Found\n", 23);
        req_fd = open("./404.html", O_RDONLY);
        is404 = 1;
    } else write(client_sockfd, "HTTP/1.1 200 OK\n", 16);

    //check if file or directory
    struct stat sb;
    fstat(req_fd, &sb);
    printf("STAT TYPE: %d\tFILE?: %d\tDIR?: %d\n", sb.st_mode, S_ISREG(sb.st_mode), S_ISDIR(sb.st_mode));

    //for directory, generate directory listing
    if (S_ISDIR(sb.st_mode) && !isIndex) {
	write_dirList(client_sockfd, req_fd, requestedPage);
        close(req_fd); //close file
        return;
    }
    //otherwise assume file
    else if (isGet) {
        //write content length string (this may not be necessary?)
        char filesizestr[32];
        sprintf(filesizestr, "%d", (int) sb.st_size);
        write(client_sockfd, "Content-length: ", 16);
        write(client_sockfd, filesizestr, strlen(filesizestr));
        write(client_sockfd, "\n", 1);

        //get file extension for page request
        char *ext;
        if ((ext = strrchr(requestedPage, '.')) > 0) printf("PAGE EXTENSION: %s\n", ext);
        //examine extension and write file type
        if (strcmp(ext, ".html") == 0 || isIndex || is404) write(client_sockfd, "Content-Type: text/html\n\n", 25);
        else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) write(client_sockfd, "Content-Type: image/jpeg\n\n", 26);
        else if (strcmp(ext, ".gif") == 0) write(client_sockfd, "Content-Type: image/gif\n\n", 25);
        else write(client_sockfd, "Content-Type: text/plain\n\n", 26);

        //attempt to send file to client socket
        if (sendfile(client_sockfd, req_fd, 0, sb.st_size) < 0) {
            printf("Server file send error\nclientfd: %d\tfilename: %s\tfilefd: %d\tfilesize: %d\n", client_sockfd, requestedPage, req_fd, (int) sb.st_size);
            perror("sendfile");
        }
    }

    close(req_fd); //close file
    return;
}

//generate a directory listing for when a user navigates to directory
//requires client socket id, requested directory file id, and specified path
void write_dirList(int client_sockfd, int req_fd, char *requestedPage){
	//DIR *d;
	//struct dirent *dir;
	printf("GENERATING DIRECTORY LISTING FOR:%s\n", requestedPage);

	write(client_sockfd, "Content-Type: text/html\n\n", 25);
	write(client_sockfd, "<html><head><title>Index of ", 28);
	write(client_sockfd, "</title></head><body><h1>Index of ", 34);
	write(client_sockfd, "</h1><address>Custom CS410 Webserver</address></body></html>", 60);
	return;
}

void exithandler(){
    //gracefully close socket file descriptor
    printf("\nClosing socket %d on port %d...", serv_sockfd, port_no);
    if (close(serv_sockfd) < 0){
        printf(" Failure!\n");
    }
    else printf(" Success!\n");
    exit(0);
}
