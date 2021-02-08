/*
tcp ipv4 server 多进程版本
*/
#include<stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include<errno.h>
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

int main(int argc, char** argv) {
    int listenfd,connfd;
    pid_t childpid;
    struct sockaddr_in servaddr; //sockaddr_in is used to describe internet(IPV4) socket address
    char buf[MAX_BUF_SIZE];
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

    while(1) {

		struct sockaddr_in client_addr;
		socklen_t cliaddr_len = sizeof(client_addr);

        //accept will block until a client connect to the server
		// 取出客户端已完成的连接
		connfd = accept(listenfd, (struct sockaddr*)&client_addr, &cliaddr_len);
        if(connfd < 0) {
            perror("accept()");
            exit(-1);
        }
        printf("Connect fd = %d\n", connfd);

        childpid = fork();
        if(childpid < 0){
        	perror("fork");
        	exit(-1);
        }else if(0 == childpid){ //子进程 接收客户端的信息，并发还给客户端
            /*关闭不需要的套接字可节省系统资源，
			  同时可避免父子进程共享这些套接字
			  可能带来的不可预计的后果
			*/
			close(listenfd);   // 关闭监听套接字，这个套接字是从父进程继承过来
            str_echo(connfd);  // process the request
            exit(0);
        }
        close(connfd);         // parent closes connected socket
    }
}
