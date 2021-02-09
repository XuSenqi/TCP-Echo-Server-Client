#include<stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/select.h>
#include <errno.h>

#define MAX_BUF_SIZE 1000

int
Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout)
{
	int	n;

	if ( (n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0)
		perror("select error");
	return(n);		/* can return 0 on timeout */
}

ssize_t
Read(int fd, void *ptr, size_t nbytes)
{
	ssize_t		n;

	if ( (n = read(fd, ptr, nbytes)) == -1)
		perror("read error");
	return(n);
}

void
Write(int fd, void *ptr, size_t nbytes)
{
	if (write(fd, ptr, nbytes) != nbytes)
		perror("write error");
}

void
Shutdown(int fd, int how)
{
	if (shutdown(fd, how) < 0)
		perror("shutdown error");
}

ssize_t						/* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
/* end writen */

void
Writen(int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
		perror("writen error");
}

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

str_cli(FILE *fp, int sockfd)
{
    int			maxfdp1, stdineof;
    fd_set		rset;
    char		buf[MAX_BUF_SIZE];
    int		n;
    
    stdineof = 0;
    FD_ZERO(&rset);
    for ( ; ; ) {
    	if (stdineof == 0)
    		FD_SET(fileno(fp), &rset);
    	FD_SET(sockfd, &rset);
    	maxfdp1 = max(fileno(fp), sockfd) + 1;
    	Select(maxfdp1, &rset, NULL, NULL, NULL);
    
    	if (FD_ISSET(sockfd, &rset)) {	/* socket is readable */
    		if ( (n = Read(sockfd, buf, MAX_BUF_SIZE)) == 0) {
    			if (stdineof == 1)
    				return;		/* normal termination */
    			else
    				printf("str_cli: server terminated prematurely");
                    exit(1);
    		}
    
    		Write(fileno(stdout), buf, n);
    	}
    
    	if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable */
    		if ( (n = Read(fileno(fp), buf, MAX_BUF_SIZE)) == 0) {
    			stdineof = 1;
    			Shutdown(sockfd, SHUT_WR);	/* send FIN */
    			FD_CLR(fileno(fp), &rset);
    			continue;
    		}
    
    		Writen(sockfd, buf, n);
                sleep(1000);
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

    str_cli(stdin, sockfd);   /* do it all */
  
    // close the socket 
    printf("call close\n");
    close(sockfd); 

    return 0;
}
