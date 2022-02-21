#include "pthread.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_BUF_SIZE 1000
void Close(int fd)
{
	if (close(fd) == -1) {
        perror("Close error");
        exit(2);
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

void
Pthread_mutex_lock(pthread_mutex_t *mptr)
{
	int		n;

	if ( (n = pthread_mutex_lock(mptr)) == 0)
		return;
	perror("pthread_mutex_lock error");
}
/* end Pthread_mutex_lock */

void
Pthread_mutex_unlock(pthread_mutex_t *mptr)
{
	int		n;

	if ( (n = pthread_mutex_unlock(mptr)) == 0)
		return;
	perror("pthread_mutex_unlock error");
}

void Pthread_cond_signal(pthread_cond_t *cptr)
{
	int		n;

	if ( (n = pthread_cond_signal(cptr)) == 0)
		return;
	perror("pthread_cond_signal error");
}

void
Pthread_cond_wait(pthread_cond_t *cptr, pthread_mutex_t *mptr)
{
	int		n;

	if ( (n = pthread_cond_wait(cptr, mptr)) == 0)
		return;

	perror("pthread_cond_wait error");
}

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

void* thread_main(void *arg)
{
	int		connfd;

	printf("thread %d starting\n", (int) arg);
	for ( ; ; ) {
        //Pthread_mutex_lock(&clifd_mutex);
        while (iget == iput) {
            Pthread_cond_wait(&clifd_cond, &clifd_mutex); //条件变量的用法见 Unix 网络编程卷1 26.8
        }
        connfd = clifd[iget];	/* connected socket to service */
        if (++iget == MAXNCLI) {
            iget = 0;
        }
        Pthread_mutex_unlock(&clifd_mutex);
        tptr[(int) arg].thread_count++;

        str_echo(connfd);		/* process request */
        Close(connfd);
	}
}

void thread_make(int i)
{
	Pthread_create(&tptr[i].thread_tid, NULL, &thread_main, (void *) i);
	return;		/* main thread returns */
}
