#include<stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>

#define MAXLINE 1000

char* Fgets(char *ptr, int n, FILE *stream)
{
	char	*rptr;

	if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream))
        perror("fgets error");

	return (rptr);
}

void Fputs(const char *ptr, FILE *stream)
{
	if (fputs(ptr, stream) == EOF)
		perror("fputs error");
}

static int	read_cnt;
static char	*read_ptr;
static char	read_buf[MAXLINE];

static ssize_t
my_read(int fd, char *ptr)
{

	if (read_cnt <= 0) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return(-1);
		} else if (read_cnt == 0)
			return(0);
		read_ptr = read_buf;
	}

	read_cnt--;
	*ptr = *read_ptr++;
	return(1);
}

ssize_t
readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else
			return(-1);		/* error, errno set by read() */
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}

ssize_t
Readline(int fd, void *ptr, size_t maxlen)
{
	ssize_t		n;

	if ( (n = readline(fd, ptr, maxlen)) < 0)
		printf("readline error");
	return(n);
}


void str_cli(FILE *fp, int sockfd)
{
    char sendline[MAXLINE], recvline[MAXLINE];

	while (Fgets(sendline, MAXLINE, fp) != NULL) {
        printf("fgets done\n"); 

        if( write(sockfd, sendline, sizeof(sendline) < 0)) {
            perror("write error");
        }
        printf("write done\n"); 

        // 正常流程
        printf("before read: sizeof(recvline) %d\n",  sizeof(recvline)); 
        //read(sockfd, recvline, MAXLINE); 
        //目前有问题，会卡在这里...................................
        Readline(sockfd, recvline, MAXLINE); 
        printf("after read: sizeof(recvline) %d\n",  sizeof(recvline)); 

		Fputs(recvline, stdout);
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

    str_cli(stdin, sockfd);   /* do it all */
  
    // close the socket 
    printf("call close\n");
    close(sockfd); 

    return 0;
}
