/*
select_echo_server_event_driven: 
参考《深入理解计算机系统》 12.2.1 基于I/O多路复用的并发事件驱动服务器
与 select_echo_server的实现没有区别，只是包装了一个pool结构 

tcp ipv4 server select版本

避免了为每个可出创建一个进程的开销
不过依然存在问题.....

1 《Unix网络编程》卷1 第3版 6.8 拒绝服务型攻击 一节上说如果一个客户端连接后，发送一个字节的数据（不是换行符）后进入sleep，server会卡在read上，不能为其他client服务。

2 《Unix网络编程》卷1 第3版 16.6 非阻塞accept 一节上说如果客户端在建立连接后，立刻发送一个RST到服务器；服务器会可能一直阻塞在accept调用上，无法为其他连接服务

解决办法：
非阻塞I/O: read/accept

*/
#include<stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include<errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define LISTENQ 50
#define IPADDR "0.0.0.0"
#define PORT 8080

#define	MAXLINE	4096	/* max text line length */

typedef struct { /* Represents a pool of connected descriptors */ //line:conc:echoservers:beginpool
    int maxfd;        /* Largest descriptor in read_set */   
    fd_set read_set;  /* Set of all active descriptors */
    fd_set ready_set; /* Subset of descriptors ready for reading  */
    int nready;       /* Number of ready descriptors from select */   
    int maxi;         /* Highwater index into client array */
    int clientfd[FD_SETSIZE];    /* Set of active descriptors */
} pool; //line:conc:echoservers:endpool
/* $end echoserversmain */
void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);
void check_clients(pool *p);

/* $begin init_pool */
void init_pool(int listenfd, pool *p) 
{
    /* Initially, there are no connected descriptors */
    int i;
    p->maxi = -1;                   //line:conc:echoservers:beginempty
    for (i=0; i< FD_SETSIZE; i++)  {
	    p->clientfd[i] = -1;        //line:conc:echoservers:endempty
    }

    /* Initially, listenfd is only member of select read set */
    p->maxfd = listenfd;            //line:conc:echoservers:begininit
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set); //line:conc:echoservers:endinit
}
/* $end init_pool */

/* $begin add_client */
void add_client(int connfd, pool *p) 
{
    int i;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++) {  /* Find an available slot */
	    if (p->clientfd[i] < 0) { 
	        /* Add connected descriptor to the pool */
	        p->clientfd[i] = connfd;                 //line:conc:echoservers:beginaddclient
	        //Rio_readinitb(&p->clientrio[i], connfd); //line:conc:echoservers:endaddclient

	        /* Add the descriptor to descriptor set */
	        FD_SET(connfd, &p->read_set); //line:conc:echoservers:addconnfd

	        /* Update max descriptor and pool highwater mark */
	        if (connfd > p->maxfd) //line:conc:echoservers:beginmaxfd
		        p->maxfd = connfd; //line:conc:echoservers:endmaxfd
	        if (i > p->maxi)       //line:conc:echoservers:beginmaxi
		        p->maxi = i;       //line:conc:echoservers:endmaxi
	        break;
	    }
    }
    if (i == FD_SETSIZE) { /* Couldn't find an empty slot */
        perror("too many clients");
        exit(-2);
    }
}
/* $end add_client */

/* $begin check_clients */
void check_clients(pool *p) 
{
    int i, connfd, n;
    char buf[MAXLINE]; 

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
	    connfd = p->clientfd[i];

	    /* If the descriptor is ready, echo a text line from it */
	    if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) { 
	        p->nready--;
	        if ((n = read(connfd, buf, MAXLINE)) == 0) {
				/*connection closed by client */
				close(connfd);
				FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            } else {
                write(connfd, buf, n);
            }
	    }
    }
}
/* $end check_clients */

int main(int argc, char** argv) {
    int					listenfd, connfd;
    ssize_t				n;
    char				buf[MAXLINE];
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;

    static pool pool; 

    int ret = 0;

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

    init_pool(listenfd, &pool); //line:conc:echoservers:initpool

	for ( ; ; ) {
        pool.ready_set = pool.read_set;
		pool.nready = select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);
        if( pool.nready < 0 ) {
            perror("select error");
            exit(-1);
        }

		if (FD_ISSET(listenfd, &pool.ready_set)) {	/* new client connection */
			clilen = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
            if(connfd < 0) {
                perror("accept()");
                exit(-1);
            }
			printf("new client: %s, port %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, 4, NULL), ntohs(cliaddr.sin_port));

            //add_client
            add_client(connfd, &pool);
		}

        check_clients(&pool);
	}
}