/*
tcp ipv4 server 多线程版本

目前有问题，accept会core

编译：
gcc server.c -lpthread -g
-g: 加入debug信息
*/

#include<stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include "pthread.h"
#include "pthread.c"

#define LISTENQ 50
#define IPADDR "0.0.0.0"
#define PORT 8080

int nthreads = 10;

extern void Pthread_mutex_lock(pthread_mutex_t *mptr);
extern void Pthread_mutex_unlock(pthread_mutex_t *mptr);
extern void Pthread_cond_signal(pthread_cond_t *cptr);

extern int get, put;

void* Calloc(size_t n, size_t size)
{
	void	*ptr;

	if ( (ptr = calloc(n, size)) == NULL) {
		perror("calloc error");
    }
	return(ptr);
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
	int		n;

again:
	if ( (n = accept(fd, sa, salenptr)) < 0) {
#ifdef	EPROTO
		if (errno == EPROTO || errno == ECONNABORTED)
#else
		if (errno == ECONNABORTED)
#endif
			goto again;
		else
			perror("accept error");
            exit(-2);
	}
	return(n);
}

int main(int argc, char** argv) {
    int i, listenfd, connfd;
    pid_t childpid;
    struct sockaddr_in client_addr,servaddr; //sockaddr_in is used to describe internet(IPV4) socket address
    int ret = 0;
    pthread_t tid;
    void thread_make(int);

    tptr = Calloc(nthreads, sizeof(Thread));
    iget = iput = 0;

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
    servaddr.sin_addr.s_addr =  htonl(INADDR_ANY);

    //Convert unsigned short to on-wire data
    servaddr.sin_port = htons(PORT);

    ///* 4create all the threads */
    for (i = 0; i < nthreads; i++) {
        thread_make(i);		/* only main thread returns */
    }

    //Let socket bind to the server address and port
    ret = bind(listenfd, (struct sockaddr *)&servaddr,sizeof(struct sockaddr_in));
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

		socklen_t cliaddr_len = sizeof(client_addr);

        //accept will block until a client connect to the server
		// 取出客户端已完成的连接
        connfd = Accept(listenfd, (struct sockaddr*)&servaddr, &cliaddr_len);
        /*
        注： 不能使用 
            int *connfdp; 
            *connfdp = Accept(listenfd, (struct sockaddr*)&servaddr, &cliaddr_len);
            这里指针 connfdp 还没有指向某个int数据，就赋值了。
            犯了 Uninitialised value was created by a stack allocation
        */
        printf("Connect fd = %d\n", connfd);

        Pthread_mutex_lock(&clifd_mutex);
        clifd[iput] = connfd;
		if (++iput == MAXNCLI) {
			iput = 0;
        }
		if (iput == iget) {
			printf("iput = iget = %d", iput);
            exit(-2);
        }
        printf("lock = %d, iput = %d\n", 111, iput);
		Pthread_cond_signal(&clifd_cond);
		Pthread_mutex_unlock(&clifd_mutex);
    }
}