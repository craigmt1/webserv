#include <stdio.h>
#include <stdlib.h>

struct cacheFile{
	char name[128];
	char *data;
	long fsize;
	struct cacheFile *next;
	struct cacheFile *prev;
};

void printCache(struct cacheFile *webcache){
	printf("Name: %s\tSize: %d bytes.\n", webcache->name, webcache->fsize);
	if (webcache->next != NULL) printCache(webcache->next);
}

struct cacheFile *readFile(char *filename){
	struct cacheFile *f1 = (struct cacheFile *) malloc(sizeof(struct cacheFile));

	strcpy(f1->name, filename);
	FILE *f = fopen(filename, "r");
	fseek(f, 0, SEEK_END);
	f1->fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	f1->data = malloc(f1->fsize + 1);
	fread(f1->data, f1->fsize, 1, f);
	fclose(f);

	f1->data[f1->fsize] = 0;

	return f1;
}

struct cacheFile *push(struct cacheFile *webcache, struct cacheFile *newfile){
	//not in cache
	if (inCache(webcache, newfile->name) == 0){
		newfile->next = webcache;
		webcache->prev = newfile;
		trimCache(newfile, 100);
		return newfile;
	}
}

int inCache(struct cacheFile *webcache, char * filename){
	if (strcmp(webcache->name, filename) == 0) return 1;
	else {
		if (webcache->next == NULL) return 0;
		else return inCache(webcache->next, filename);
	}
}

int cacheSize(struct cacheFile *webcache){
	if (webcache->next == NULL) return 1;
	else return 1 + cacheSize(webcache->next);
}

//finds cache file with given name, and pushes it back to front
//returns 1 to signal that there is new front
int writeCacheFile(struct cacheFile *front, struct cacheFile *webcache, char * filename){
	//if first print and do nothing
	if (strcmp(webcache->name, filename) == 0){
		printf("%s\n", webcache->data);
		if (webcache->prev == NULL) {
			return 0;
		}
		else {
			if (webcache->next){
				webcache->prev->next = webcache->next;
				webcache->next->prev = webcache->prev;
			} else webcache->prev->next = NULL;
			webcache->prev = NULL;
			webcache->next = front;
			front->prev = webcache;
			return 1;
		}
	}
	else {
		if (webcache->next == NULL) return 0;
		else return writeCacheFile(front, webcache->next, filename);
	}
}

//keep cache below certain length, free items at end that go beyond it
int trimCache(struct cacheFile *webcache, int max){
	if (max == 0){
		webcache->prev->next = NULL;
		free(webcache->data);
		free(webcache);
		return 1;
	}
	else if (webcache->next == NULL){
		return 0;
	}
	else{
		return trimCache(webcache->next, max - 1);
	}
}

int main(){
	struct cacheFile *webcache;

	//if nothing in cache yet
	if (webcache == NULL) webcache = readFile("notes.txt");

	webcache = push(webcache, readFile("bonusideas.txt"));

	printCache(webcache);
	printf("Size of Cache: %d\n", cacheSize(webcache));
	printf("%s in Cache? %d\n", "notes.txt", inCache(webcache, "notes.txt"));
	printf("%s in Cache? %d\n", "lol", inCache(webcache, "lol"));

	if (writeCacheFile(webcache, webcache, "webserv.c")){
		webcache = webcache->prev;
	} else{
		//push cache file then write it
		webcache = push(webcache, readFile("webserv.c"));
		writeCacheFile(webcache, webcache, "webserv.c");
	}


	printf("Size of Cache: %d\n", cacheSize(webcache));
	printCache(webcache);

	free(webcache->data);
	free(webcache);
	return 0;
}