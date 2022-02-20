/*
gcc server.c threadpool.c -lpthread
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "threadpool.h"

#define ser_ip "127.0.0.1"
#define ser_port 3698
#define max_thread_size 20  //初始化线程池的大小

/*
 *执行线程的参数（一个结构体）
 */
struct info
{
	int client_sock;                
	struct sockaddr_in client_addr;
};

/*
 *线程的工作（接受客户端的数据并返回小写）
 */
void* do_work(void* arg)
{
	char buf[BUFSIZ],str[BUFSIZ];
	struct info* ts = (struct info*)arg;

	//打印客户端的信息
	printf("======the connect client ip %s,port %d\n",
					inet_ntop(AF_INET,&(*ts).client_addr.sin_addr.s_addr,str,sizeof(str)),
					ntohs((*ts).client_addr.sin_port));
	while(1)
	{
		int len = read(ts->client_sock,buf,sizeof(buf));
		
		if(len == 0)
		{
			close(ts->client_sock);
			break;
		}
		else if(len>0)
		{
            int i;
			for(i = 0;i<len;i++)
				buf[i] = toupper(buf[i]);
			write(ts->client_sock,buf,len);
		}
	}
	return NULL;
}

int main()
{
	struct sockaddr_in ser_addr,client_addr;
	int serv_sock,client_sock;
	socklen_t client_sz; 
	thread_pool * pool;
	struct info ts[256];
	int i = 0;

	pool = (thread_pool*)malloc(max_thread_size*sizeof(thread_pool));

	serv_sock = socket(AF_INET,SOCK_STREAM,0);
	bzero(&ser_addr,sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ser_addr.sin_port = htons(ser_port);

	thread_pool_init(pool,max_thread_size);
	
	if(bind(serv_sock,(struct sockaddr*)& ser_addr,sizeof(ser_addr)) == -1)
	{
		perror("bind() ");
		exit(1);
	}

	if(listen(serv_sock,20) == -1)
	{
		perror("listen() ");
		exit(1);
	}

	//while(int j = 0;j<max_thread_size;j++)
	while(1)
	{
		client_sz = sizeof(client_addr);
		client_sock = accept(serv_sock,(struct sockaddr*)& ser_addr,&client_sz);
		ts[i].client_sock = client_sock;
		ts[i].client_addr = client_addr;

		//把连接的客户端加入给等待队列中的线程处理
		threadpool_add_runner(pool,do_work,(void*)&ts[i]);
		i++;
	}
	return 0;
}