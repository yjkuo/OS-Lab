#include "client.h"

#define MAX_SIZE 8192
int main(int argc, char *argv[])
{
    int sockfd;
    int opt,n;
    char file[20],host[10],port[10];
    char req[100],recvline[MAX_SIZE];
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM,0);
    while((opt = getopt(argc,argv,"t:h:p:")) != -1) {
        switch(opt) {
        case 't':
            strcpy(file,optarg);
            break;
        case 'h':
            strcpy(host,optarg);
            break;
        case 'p':
            strcpy(port,optarg);
            break;
        case '?':
            printf("Using getopt -t QUERY_FILE_OR_DIR -h LOCALHOST -p PORT\n");
            break;
        }
    }
    sprintf(req,"GET %s HTTP/1.x\r\nHOST: %s:%s\r\n",file,host,port);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(port));

    inet_pton(AF_INET,host,&(servaddr.sin_addr));
    connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    bzero(recvline,sizeof(recvline));
    write(sockfd,req,strlen(req)+1);
    n = read(sockfd,recvline,8192);
    printf("%s\n",recvline);

    return 0;
}

