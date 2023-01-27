#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>


static void *threadpool_thread(void *tpool);
static void addq(void *tpool,char *req);
static void handle(void *pool,char *req);
static int is_dir(char *path);
static char *content_type(char *path);



#endif
