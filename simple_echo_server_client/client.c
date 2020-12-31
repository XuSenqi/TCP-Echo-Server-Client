#include<stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>

#define MAX_BUF_SIZE 1000

void process(int sockfd) 
{ 
    char buff[MAX_BUF_SIZE]; 
    int n; 
    for (;;) { 
        bzero(buff, sizeof(buff)); 
        printf("Enter the string : "); 
        n = 0; 
        while ((buff[n++] = getchar()) != '\n') 
            ; 
        write(sockfd, buff, sizeof(buff)); 
        bzero(buff, sizeof(buff)); 
        read(sockfd, buff, sizeof(buff)); 
        printf("From Server : %s", buff); 
        if ((strncmp(buff, "exit", 4)) == 0) { 
            printf("Client Exit...\n"); 
            break; 
        } 
    } 
} 

int main(int argc, char** argv) {
	int					sockfd;
	struct sockaddr_in	servaddr;

    if (argc != 3) {
        printf("usage: %s <IPaddress> <Port>\n", argv[0]); 
        exit(1);
    }
    int port = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("socket() error"); //perror - write error messages to standard error
        exit(2);
    }

    bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
    int ret = inet_aton(argv[1], &servaddr.sin_addr);
    if(ret < 0) {
        perror("inet_aton() error");
        exit(3);
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) != 0) {
        perror("connect error");
        exit(1);
    }

    // function for chat 
    process(sockfd); 
  
    // close the socket 
    close(sockfd); 

    return 0;
}
