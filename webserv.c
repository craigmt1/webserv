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

struct sockaddr_in addr; //server socket address structure
socklen_t addrlen; //address structure size

static const char site_directory[128] = "./www/"; //path of site resources
static const char error_directory[128] = "./www/err/"; //path of error code html files

static int serv_sockfd; //server socket file descriptor
static size_t serv_sock_size; //size of server socket file descriptor
static int port_no; //port number for webserv
static const int req_buf_size = 1024; //client request buffer size

void client_request(int client_sockfd);
void write_file(int client_sockfd, char *req_path, int filesize);
void write_dir(int client_sockfd, char *req_path);
void write_cgi(int client_sockfd, char *req_path, char *cgi_args);
void write_error(int client_sockfd, int error_code);
void exithandler();

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

    chdir(site_directory); //change working directory to website directory

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
				exit(0);
				close(client_sockfd);
		    }
        }
    }
    exithandler();
    return 0;
}

//handle single request from a given client socket
void client_request(int client_sockfd){
    //initialize request buffer
	char req_buf[req_buf_size];

    //load messages from client_sockfd into request buffer http://linux.die.net/man/2/recv
    recv(client_sockfd, req_buf, req_buf_size, 0); //not really sure what to do with flag arg
    printf("Request from client socket %d\n%s", client_sockfd, req_buf); //output request message to console
    
    //tokenize request buffer (first token is request type)
    char * req_tokens = strtok(req_buf, " ");
    
    //close if not GET request
    if (strcmp(req_tokens, "GET") != 0){
    	printf("ERROR: Expected GET, received: %s\n", req_tokens);
    	write_error(client_sockfd, 404);
    	return;
    }

    //store next token (PATH), redirect to index.html if root dir
    req_tokens = strtok(NULL, " ");
    char req_path[256] = ".";
    if (strcmp(req_tokens, "/") == 0) strcat(req_path, "/index.html");
    else strcat(req_path + 1, req_tokens);
    
    //check path url for cgi arguments, separate them for stat
    char *path_token;
    char cgi_args[256] = "";
    if (path_token = strrchr(req_path, '?')) {
        //copy query to different string
        strcpy(cgi_args, path_token);
        //remove query from path string (add null terminator at '?')
		req_path[(int)(path_token - req_path)] = '\0';
		write(client_sockfd, "HTTP/1.1 200 OK\n", 16);
		write_cgi(client_sockfd, req_path, cgi_args);
		return;		
    }
    printf("QUERY STRING:%s\n", cgi_args);

	struct stat sb;
	if (stat(req_path, &sb) < 0) perror("checkPath: stat()");

    printf("PAGE REQUESTED: %s\n", req_path);
	switch (sb.st_mode & S_IFMT) {
		case S_IFDIR:
			write_dir(client_sockfd, req_path);
			break;
		case S_IFREG:
			write_file(client_sockfd, req_path, (int) sb.st_size);
			break;
		default:
			write_error(client_sockfd, 404);
			break;
	}
    return;
}

//parses error code, sends corresonding error code in /err/ directory if it exists using write_file
void write_error(int client_sockfd, int error_code){
	chdir("..");
	chdir(error_directory);
	char err_path[128] = "./";
    sprintf(err_path + 2, "%d", error_code);
    strcat(err_path, ".html");
    
    struct stat sb;
    if (stat(err_path, &sb) < 0) {
    	perror("ErrorHtmlFile: stat()");
    	return;
    } else write_file(client_sockfd, err_path, (int) sb.st_size);
    return;
}

void write_cgi(int client_sockfd, char *req_path, char *cgi_args){
	FILE *f;

	if (strlen(cgi_args) > 1) {
		req_path[strlen(req_path) + 1] = '\0';
		req_path[strlen(req_path)] = ' ';
		strcat(req_path, cgi_args);
	}
	printf("EXECUTING CGI SCRIPT: %s", req_path);


	if ((f = popen(req_path, "r")) < 0){
		perror("cgi:popen()");
		void write_error(int client_sockfd, int error_code);
		return;
	}
	//write(client_sockfd, "Content-length: 4096\n", 21);
	write(client_sockfd, "Connection: close\n", 18);
	//write(client_sockfd, "Content-Type: text/plain\n", 25);
	//write(client_sockfd, "\nContent-Type: text/html\n\n", 26);
	//write(client_sockfd, "HELLO\n", 6);

	//write stdout line by line
	char line[256];
	//char cgi_out[10000];
	while (fgets(line, 256, f) != NULL){
		//strcat(cgi_out, line);
		//printf("%s\n", line);
		write(client_sockfd, line, strlen(line));
	}

	//printf("CGI OUTPUT: %s\n", cgi_out);

	/*
	//write content length
    char filesizestr[32];
    sprintf(filesizestr, "%d", (int) strlen(cgi_out));
    write(client_sockfd, "Content-length: ", 16);
    write(client_sockfd, filesizestr, strlen(filesizestr));
    write(client_sockfd, "\n", 1);

    //write output
    write(client_sockfd, cgi_out, strlen(cgi_out));
	*/
	pclose(f);
	close(client_sockfd);
	return;
}

//writes a file to the client socket using a string for filename and int for size of file
void write_file(int client_sockfd, char *req_path, int filesize){
	int req_fd; //file descriptor for requested page
	char *ext; //file extension of requested page

	write(client_sockfd, "HTTP/1.1 200 OK\n", 16);

	//get file extension
	if ((ext = strrchr(req_path, '.')) > 0) printf("PAGE EXTENSION: %s\n", ext);

	//redirect to write_cgi if cgi file
	if (strcmp(ext, ".cgi") == 0) {
		write_cgi(client_sockfd, req_path, "");
		return;
	}

	printf("RETRIEVING FILE FOR:%s\tSIZE: %d\n", req_path, filesize);
	
	//init file descriptor
	req_fd = open(req_path, O_RDONLY);
	
	//content length string
    char filesizestr[32];
    sprintf(filesizestr, "%d", filesize);
    write(client_sockfd, "Content-length: ", 16);
    write(client_sockfd, filesizestr, strlen(filesizestr));
    write(client_sockfd, "\n", 1);
	
    //examine extension and write file type
    if (strcmp(ext, ".html") == 0) write(client_sockfd, "Content-Type: text/html\n\n", 25);
    else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) write(client_sockfd, "Content-Type: image/jpeg\n\n", 26);
    else if (strcmp(ext, ".gif") == 0) write(client_sockfd, "Content-Type: image/gif\n\n", 25);
    else write(client_sockfd, "Content-Type: text/plain\n\n", 26);

    //attempt to send file to client socket
    if (sendfile(client_sockfd, req_fd, 0, filesize) < 0) {
        printf("Server file send error\nclientfd: %d\tfilename: %s\tfilefd: %d\tfilesize: %d\n", client_sockfd, req_path, req_fd, filesize);
        perror("sendfile");
    }
    close(req_fd); //close file
	return;
}

//generate a directory listing for when a user navigates to directory
//requires client socket id, requested directory file id, and specified path
void write_dir(int client_sockfd, char *req_path){
	DIR *d;
	struct dirent *dir;
	printf("GENERATING DIRECTORY LISTING FOR:%s\n", req_path);

	//append slash to directory path string if it doesnt already exist
	if (req_path[strlen(req_path) - 1] != '/'){
		strcat(req_path, "/");
		printf("NEW DIR STRING: %s\n", req_path);
	}
	write(client_sockfd, "HTTP/1.1 200 OK\n", 16);

	char dirstr[10000] = "<html>\n\t<head>\n\t\t<title>Index of ";
	strcat(dirstr, req_path + 1);
	strcat(dirstr, "</title>\n\t</head>\n\t<body>\n\t\t<h1>Index of ");
	strcat(dirstr, req_path + 1);
	strcat(dirstr, "</h1>\n\t\t<table>\n\t\t\t<tr><th colspan=\"5\"><hr></th></tr>");
	//parent link
	strcat(dirstr, "\n\t\t\t<tr><td valign=\"top\"><img src=\"/icons/back.gif\" alt=\"[DIR]\"></td><td><a href=\"");
	strcat(dirstr, req_path + 1);
	strcat(dirstr, "../\">Parent Directory</a></td><td>&nbsp;</td></tr>");

	//generate links for all files in directory. render files in directory to table
	d = opendir(req_path);
	while ((dir = readdir (d)) != NULL) {
		//skip cwd and parent
		if ((strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0)) continue;
		
		//render listing for other files/directories
		strcat(dirstr, "\n\t\t\t<tr><td valign=\"top\"><img src=\"/icons/");

		//choose icon for directory or file
		if (dir->d_type == DT_DIR) strcat(dirstr, "folder.gif\" alt=\"[DIR]");
		else strcat(dirstr, "text.gif\" alt=\"[TXT]");

		strcat(dirstr, "\"></td><td><a href=\"");
		strcat(dirstr, req_path + 1);
		strcat(dirstr, dir->d_name);
		strcat(dirstr, "\">");
		strcat(dirstr, dir->d_name);
		strcat(dirstr, "</a></td><td>&nbsp;</td></tr>");
	}
	strcat(dirstr, "\n\t\t\t<tr><th colspan=\"5\"><hr></th></tr>\n\t\t</table>\n\t\t<address>Custom CS410 Webserver</address>\n\t</body>\n</html>");

	char dirstr_size[32];
	sprintf(dirstr_size, "%d", (int) strlen(dirstr));
	write(client_sockfd, "Content-length: ", 16);
	write(client_sockfd, dirstr_size, strlen(dirstr_size));
	write(client_sockfd, "\nContent-Type: text/html\n\n", 26);
	write(client_sockfd, dirstr, strlen(dirstr));

	closedir(d);
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