#include "server.h"
#include "status.h"


int portnum;
char buf[4096];
char str[100],rootpath[50];
char *requestq[50];
sem_t mutex;

typedef struct threadpool_t {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    int thread_count;
    int count;
    int in;
    int out;
    int flag;
} threadpool;

static void *threadpool_thread(void *tpool)
{
    threadpool *pool = (threadpool *)tpool;
    int i;
    char *tmp;
    while(1) {
        pthread_mutex_lock(&(pool->lock));
        while(pool->count == 0 && pool->flag != 1) {
            pthread_cond_wait(&(pool->notify),&(pool->lock));
            printf("start exit\n");
        }
        if(pool->flag)
            break;
        i = pool->out;
        tmp = requestq[i];
        pool->out = (pool->out+1)%50;
        pool->count -= 1;
        handle((void *)pool,tmp);
        pthread_mutex_unlock(&(pool->lock));
    }
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
}
static void addq(void *tpool,char *req)
{
    /* add request to queue*/
    threadpool *pool = (threadpool *)tpool;
    pthread_mutex_lock(&(pool->lock));
    sem_wait(&mutex);
    if(pool->count == 50) {
        printf("Queue is full\n");
        return;
    }
    requestq[pool->in] = req;
    //printf("NOW IN QUEUQ:%s\n",requestq[pool->in]);
    pool->in ++;
    pool->count ++;
    sem_post(&mutex);
    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

}
static void handle(void *tpool,char *req)
{
    printf("REQ:%s\n",req);//debug
    char c;
    char tmp[300];
    char temp[512];
    int i = 0;
    FILE *ifp;
    DIR *d;
    threadpool *pool = (threadpool *)tpool;
    char *tok = strtok(req," ");
    if(strcmp(tok,"GET") == 0) {
        tok = strtok(NULL," ");/* Request File Path */
        if(tok[0]=='/') {
            sprintf(tmp,"%s%s",rootpath,tok);
            ifp = fopen(tmp,"r");
            if(!ifp) {
                sprintf(temp,"HTTP/1.x %d NOT_FOUND\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n",
                        status_code[NOT_FOUND],
                        extensions[8].mime_type);
                strcat(buf,temp);
            } else {
                printf("dir is:%s\n",tmp);
                if(is_dir(tmp)) {
                    sprintf(temp,"HTTP/1.x %d OK\r\nContent-Type: directory\r\nServer: httpserver/1.x\r\n\r\n",
                            status_code[OK]);
                    strcat(buf,temp);
                    struct dirent *dir;
                    d = opendir(tmp);
                    if(d) {
                        while((dir=readdir(d)) != NULL) {
                            if(strcmp(dir->d_name,"..")!=0 &&strcmp(dir->d_name,".")!=0) {
                                sprintf(temp,"%s ",dir->d_name);
                                strcat(buf,temp);
                                sprintf(tmp,"GET %s/%s HTTP/1.x\r\nHOST: 127.0.0.1:%d\r\n\r\n",tok,dir->d_name,portnum);
                                addq(pool,tmp);
                            }
                        }
                        strcat(buf,"\r\n\r\n");
                        closedir(d);
                    }
                } else {
                    if(!content_type(tmp)) {
                        sprintf(temp,"HTTP/1.x %d UNSUPPORT_MEDIA_TYPE\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n",
                                status_code[UNSUPPORT_MEDIA_TYPE],
                                extensions[8].mime_type);
                        strcat(buf,temp);
                    }
                    sprintf(temp,"HTTP/1.x %d OK\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n",
                            status_code[OK],
                            content_type(tok));
                    strcat(buf,temp);
                    /* File Content */
                    while((c=getc(ifp))!=EOF) {
                        temp[i++]=c;
                    }
                    temp[i] = '\0';
                    strcat(buf,temp);
                    fclose(ifp);
                }
            }
        } else {
            sprintf(temp,"HTTP/1.x %d BAD_REQUEST\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n",
                    status_code[BAD_REQUEST],
                    extensions[8].mime_type);
            strcat(buf,temp);
        }

    } else {
        sprintf(temp,"HTTP/1.x %d METHOD_NOT_ALLOWED\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n",
                status_code[METHOD_NOT_ALLOWED],
                extensions[8].mime_type);
        strcat(buf,temp);
    }
    if(pool->count == 0) {
        pool->flag = 1;
        pthread_cond_broadcast(&(pool->notify));
    }

}
static int is_dir(char *path)
{
    struct stat stb;
    stat(path,&stb);
    if(S_ISREG(stb.st_mode)) {
        return 0;
    }
    if(S_ISDIR(stb.st_mode)) {
        return 1;
    }

}
static char *content_type(char *path)
{
    char *ext;
    int i;
    ext = strrchr(path,'.');
    for(i=0; i<7; ++i) {
        if(strcmp((ext+1),extensions[i].ext)==0) {
            return extensions[i].mime_type;
        }
    }
    return NULL;
}
int main(int argc, char *argv[])
{
    int listen_fd, comm_fd;
    int opt,i,thread_count=0;
    threadpool *pool = (threadpool *)malloc(sizeof(threadpool));

    while((opt = getopt(argc,argv,"r:p:n:")) != -1) {
        switch(opt) {
        case 'r':
            strcpy(rootpath,optarg);
            break;
        case 'p':
            portnum = atoi(optarg);
            break;
        case 'n':
            thread_count = atoi(optarg);
            break;
        case '?':
            printf("Using getopt -r root -p port -n thread number\n");
            break;
        }
    }
    /* Initialize */
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->in = pool->count = pool->out = 0;
    pool->thread_count = 0;
    pool->flag = 0;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(pool->lock),&attr);
    pthread_cond_init(&(pool->notify),NULL);
    sem_init(&mutex,0,1);
    for(i = 0; i<thread_count; ++i) {
        pthread_create(&(pool->threads[i]),NULL,threadpool_thread,(void *)pool);
        pool->thread_count++;
    }

    struct sockaddr_in servaddr;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    bzero( &servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(portnum);

    bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    listen(listen_fd, 10);

    comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);

    bzero( str, 100);

    read(comm_fd,str,100);
    printf("%s\n",str);
    addq((void *)pool,str);

    printf("waiting for threads complete......\n");
    for(i = 0; i<pool->thread_count; ++i) {
        pthread_join(pool->threads[i],NULL);
    }

    write(comm_fd, buf, strlen(buf)+1);
    /* deallocate */
    free(pool->threads);
    pthread_mutex_lock(&(pool->lock));
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    sem_destroy(&mutex);
    free(pool);
    return 0;

}


