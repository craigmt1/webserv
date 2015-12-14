struct cacheFile *readFile(char *filename);
struct webCache *create(long maxSize);
void clear(struct cacheFile *fc);
void clearWebCache(struct webCache *wc);
struct cacheFile *inCache(struct cacheFile *fc, char * filename);
struct cacheFile *cache(struct webCache *wc, char *filename);
void printCache(struct cacheFile *fc);
void printWebCache(struct webCache *wc);

void client_request(int client_sockfd);
void write_file(int client_sockfd, char *req_path, int filesize);
void write_dir(int client_sockfd, char *req_path);
void write_cgi(int client_sockfd, char *req_path, char *cgi_args);
void write_error(int client_sockfd, int error_code);
void exithandler();

//struct for files in cache
struct cacheFile{
	char name[128];
	char *data;
	long fsize;
	struct cacheFile *next;
	struct cacheFile *prev;
};

//struct for managing all cache files, pointer to head and tail plus file size info
struct webCache{
	struct cacheFile *head;
	struct cacheFile *tail;
	long maxSize;
	long size;
};
