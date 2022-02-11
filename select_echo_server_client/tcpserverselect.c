/*
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

int main(int argc, char** argv) {
    int					i, maxi, maxfd, listenfd, connfd, sockfd;
    int					nready, client[FD_SETSIZE];
    ssize_t				n;
    fd_set rset,allset;
    char				buf[MAXLINE];
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;

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

   	maxfd = listenfd;			/* initialize */
	maxi = -1;					/* index into client[] array */
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;			/* -1 indicates available entry */
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

    /* include fig02 */
	for ( ; ; ) {
		rset = allset;		/* structure assignment */
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        if( nready < 0 ) {
            perror("select error");
            exit(-1);
        }

		if (FD_ISSET(listenfd, &rset)) {	/* new client connection */
			clilen = sizeof(cliaddr);
            connfd = accept(listenfd, NULL, NULL);
            if(connfd < 0) {
                perror("accept()");
                exit(-1);
            }
			printf("new client: %s, port %d\n",
					inet_ntop(AF_INET, &cliaddr.sin_addr, 4, NULL),
					ntohs(cliaddr.sin_port));

			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd;	/* save descriptor */
					break;
				}
			if (i == FD_SETSIZE) {
				perror("too many clients");
                exit(-2);
            }

			FD_SET(connfd, &allset);	/* add new descriptor to set */
			if (connfd > maxfd)
				maxfd = connfd;			/* for select */
			if (i > maxi)
				maxi = i;				/* max index in client[] array */

			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}

		for (i = 0; i <= maxi; i++) {	/* check all clients for data */
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ( (n = read(sockfd, buf, MAXLINE) ) == 0) {
						/*4connection closed by client */
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				} else
					write(sockfd, buf, n);

				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}
}