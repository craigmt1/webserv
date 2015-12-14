#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filecache.h"

//generate cache file from filename
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

//initialize web cache
struct webCache *create(long maxSize){
	struct webCache *wc = (struct webCache *) malloc(sizeof(struct webCache));
	wc->maxSize = maxSize;
	wc->size = 0;
	return wc;
}

//empty cache starting at given file in cache (start at head to clear all)
void clear(struct cacheFile *fc){
	if (fc->next != NULL) clear(fc->next);
	fc->fsize = 0;
	fc->next = NULL;
	fc->prev = NULL;
	
	free(fc->data);
	free(fc);
	fc = NULL;
}
void clearWebCache(struct webCache *wc){
	clear(wc->head);
	wc->size = 0;
	wc->head = NULL;
	wc->tail = NULL;
}

//checks if item is in cache, returns item if true
struct cacheFile *inCache(struct cacheFile *fc, char * filename){
	if (strcmp(fc->name, filename) == 0) return fc;
	else {
		if (fc->next == NULL) return NULL;
		else return inCache(fc->next, filename);
	}
}

//caches or returns existing file to web cache
struct cacheFile *cache(struct webCache *wc, char *filename){
	struct cacheFile *cf;

	//if first item
	if (wc->head == NULL){
		cf = readFile(filename);
		if (cf->fsize > wc->maxSize){
			free(cf);
			return NULL;
		}
		wc->head = cf;
		wc->tail = cf;
		wc->size = cf->fsize;
		return wc->head;
	}

	//else if not yet in cache, make head and trim cache
	cf = inCache(wc->head, filename);
	if(cf == NULL){
		cf = readFile(filename);
		if (cf->fsize > wc->maxSize){
			free(cf);
			return NULL;
		}
		cf->next = wc->head;
		wc->head->prev = cf;
		wc->head = cf;
		wc->size = wc->size + cf->fsize;
		//remove tail items until under size limit
	} else {
		//exit if already at head
		if (cf->prev == NULL) return;
		//move tail to prev if at tail
		if (cf->next == NULL){
			wc->tail = cf->prev;
			wc->tail->next = NULL;
		//if between two items, connect them to each other
		} else {
			cf->next->prev = cf->prev;
			cf->prev->next = cf->next;
		}
		//move to head
		wc->head->prev = cf;
		cf->next = wc->head;
		cf->prev = NULL;
		wc->head = cf;
	}
	//trim cache if over limit
	while (wc->size > wc->maxSize){
		wc->size = wc->size - wc->tail->fsize;
		wc->tail = wc->tail->prev;
		clear(wc->tail->next);
		wc->tail->next = NULL;
	}
	return wc->head;
}

//recursively print file info for files in cache
void printCache(struct cacheFile *fc){
	printf("Size: %d\tName: %s\t", fc->fsize, fc->name);
	if (fc->prev != NULL){
		printf("Prev: %s\t", fc->prev->name);
	} else printf("Prev: NULL\t");

	if (fc->next != NULL){
		printf("Next: %s\n", fc->next->name);
		printCache(fc->next);
	} else printf("Next: NULL\n");
}
//print cache info (and perform recursive file print)
void printWebCache(struct webCache *wc){
	printf("Memory usage: (%u/%u)\n", wc->size, wc->maxSize);
	if (wc->head != NULL) printCache(wc->head);
}

/*
int main(){
	struct webCache *wc;
	wc = create(12000);
	cache(wc, "notes.txt");
	cache(wc, "bonusideas.txt");
	cache(wc, "www/request.cgi");
	cache(wc, "www/other.html");
	cache(wc, "www/test.html");
	cache(wc, "www/dir.html");
	cache(wc, "www/test.cgi");
	cache(wc, "www/favicon.ico");
	cache(wc, "www/binduino.cgi");
	cache(wc, "www/display-histogram.cgi");
	cache(wc, "notes.txt");

	printWebCache(wc);
	clearWebCache(wc);
	return 0;
}
*/
