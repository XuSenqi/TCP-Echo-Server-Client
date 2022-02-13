/*
tcp ipv4 server 多线程版本

编译：
gcc server.c -lpthread
*/

#include<stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include<errno.h>
#include <pthread.h>
//#include <unistd.h>

#define MAX_BUF_SIZE 1000
#define LISTENQ 50
#define IPADDR "0.0.0.0"
#define PORT 8080

void str_echo(int sockfd)
{
	int	n;
    int ret = 0;
    char  buf[MAX_BUF_SIZE];

again:
    //read from the client 
    //if client calls close or is killed normally(即server端读取了全部数据了), read returns 0
    //if client 接收缓冲区里数据还没有完全被读取，就 calls close or is killed normally, client发送rst给server， server read returns -1, errno 被设置为104(ECONNRESET，详细定义见errno.h)
    //if server read was interrupted by a signal before any data was read, read returns -1, errno被置为EINTR, 需要重复读取
	while ( (n = read(sockfd, buf, MAX_BUF_SIZE)) > 0) {
        //write back to the client
        ret = write(sockfd, buf, strlen(buf) * sizeof(char));
        if(ret < 0) {
            perror("write()");
            break;
        }
    }

	if (n < 0 && errno == EINTR) {
		goto again;
    }
	else if (n < 0) {
		printf("str_echo: read %d, errno %d\n", n, errno);
    }
}

void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, 
		    void * (*routine)(void *), void *argp) 
{
    int rc;

    if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0) {
        perror("Pthread_create error");
        exit(2);
    }
}

/* $begin detach */
void Pthread_detach(pthread_t tid) {
    int rc;

    if ((rc = pthread_detach(tid)) != 0) {
        perror("Pthread_detach error");
        exit(2);
    }
}
/* $end detach */

void Close(int fd)
{
	if (close(fd) == -1) {
        perror("Close error");
        exit(2);
    }
}

/* Thread routine */
void *thread(void *varg) 
{  
    int connfd = *((int *)varg);
    Pthread_detach(pthread_self()); //line:conc:echoservert:detach
    str_echo(connfd);
    Close(connfd);
    return NULL;
}

int main(int argc, char** argv) {
    int listenfd, *connfdp;
    pid_t childpid;
    struct sockaddr_in servaddr; //sockaddr_in is used to describe internet(IPV4) socket address
    char buf[MAX_BUF_SIZE];
    int ret = 0;
    pthread_t tid;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) {
        perror("socket() error"); //perror - write error messages to standard error
        exit(2);
    }

    //Set all bytes to zero
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;  

    //inet_aton convert string based address to binary data
    //Do not use inet_addr, for more info see man page
    ret = inet_aton(IPADDR, &servaddr.sin_addr);
    if(ret < 0) {
        perror("inet_aton() error");
        exit(3);
    }

    //You can also choose the code below to let socket
    //listen to all interface
    //servaddr.sin_addr.s_addr =  htonl(INADDR_ANY);

    //Convert unsigned short to on-wire data
    servaddr.sin_port = htons(PORT);

    //Let socket bind to the server address and port
    ret = bind(listenfd, (const struct sockaddr *)&servaddr,sizeof(struct sockaddr_in));
    if(ret < 0) {
        perror("bind()");
        exit(-1);
    }

    //Listen for incoming connections
    ret = listen(listenfd, LISTENQ);
    if(ret < 0) {
        perror("listen()");
        exit(-1);
    }
    printf("Server listening in %s:%d\n", IPADDR, PORT);

    while(1) {

		struct sockaddr_in client_addr;
		socklen_t cliaddr_len = sizeof(client_addr);

        //accept will block until a client connect to the server
		// 取出客户端已完成的连接
		*connfdp = accept(listenfd, (struct sockaddr*)&client_addr, &cliaddr_len);
        if(*connfdp < 0) {
            perror("accept()");
            exit(-1);
        }
        printf("Connect fd = %d\n", *connfdp);

        Pthread_create(&tid, NULL, thread, (void *)connfdp);
    }
}