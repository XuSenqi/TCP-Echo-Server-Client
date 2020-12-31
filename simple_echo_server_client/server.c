/*
tcp ipv4 server simple example
只是简单的demo, 无法线上使用

存在的问题：
1 每次只能处理一个client连接，其他的client卡在等待server的accept

*/
#include<stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>

#define MAX_BUF_SIZE 1000
#define LISTENQ 50
#define IPADDR "127.0.0.1"
#define PORT 8080

int main(int argc, char** argv) {
    int listenfd, connfd;
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
        //accept will block until a client connect to the server
        connfd = accept(listenfd, NULL, NULL);
        if(connfd < 0) {
            perror("accept()");
            exit(-1);
        }
        printf("Connect fd = %d\n", connfd);
        memset(buf, 0, sizeof(buf));

        //read from the client until client close/or send EOF
        while((ret = read(connfd, buf, (size_t)MAX_BUF_SIZE)) && ret != EOF) {
            if(ret < 0) {
                perror("read()");
                exit(-1);
            }
            printf("Get data %s\n", buf);

            //write back to the client
            ret = write(connfd, buf, strlen(buf) * sizeof(char));
            if(ret < 0) {
                perror("write()");
                break;
            }
        }
        close(connfd);
    }

    return 0;
}