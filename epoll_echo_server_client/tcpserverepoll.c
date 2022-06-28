/*
tcp ipv4 server epoll版本

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
#include <errno.h>
#include <sys/epoll.h>   //注：epoll是linux独有的，Mac电脑上并没有sys/epoll.h头文件
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXEVENTS 128
#define SERV_PORT 12345

void nonblocking_server(int port){}

int main(int argc, char **argv) {
    int listen_fd, socket_fd;
    int n, i;
    int efd;

    struct epoll_event event;
    struct epoll_event *events;

    listen_fd = nonblocking_server(SERV_PORT);
    efd = epoll_create1(0);//调用 epoll_create0 创建了一个 epoll 实例。
    if (efd == -1) {
        error(1, errno, "epoll create failed");
    }
  
  /*
  调用 epoll_ctl 将监听套接字对应的 I/O 事件进行了注册，这样在有新的连接建立之后
  就可以感知到。注意这里使用的是 edge-triggered（边缘触发）。
  */
    event.data.fd = listen_fd;
    event.events = EPOLL | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {
        error(1, errno, "epoll_ctl add listen fd failed");
    }
    // Buffer where events are returned 为返回的 event 数组分配了内存。
    events = calloc(MAXEVENTS, sizeof(event));

    //主循环调用 epoll_wait 函数分发 I/O 事件
    //当 epoll_wait 成功返回时，通过遍历返回的 event 数组，就直接可以知道发生的 I/O 事件。
    while (1) {
        n = epoll_wait(efd, events, MAXEVENTS, -1);
        printf("epoll_wait wakeup\n");
    
        for(i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN))) {
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
            } else if (listen_fd == events[i].data.fd) {
               /*
               监听套接字上有事件发生的情况下，调用 accept 获取已建立连接
               并将该连接设置为非阻塞，再调用 epoll_ctl 把已连接套接字对应的可读事件
               注册到 epoll 实例中。
               这里我们使用了 event_data 里面的 fd 字段，将连接套接字存储其中。
               注意监听套接字和连接套接字不一样。
               */
                struct sockaddr_storage ss;
                socklen_t slen = sizeof(ss);
                int fd = accept(listen_fd, (struct sockaddr *)&ss, &slen);
                if (fd < 0) {
                    error(1, error, "accept failed");
                } else {
                    nonblocking_server(fd);
                    event.data.fd = fd;
                    event.events = EPOLLIN | EPOLLET; //edge-triggered
                    if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event) == -1) {
                        error(1, errno, "epoll_ctl add connection fd failed");
                    }
                }
                continue;
            } else {
                //处理了已连接套接字上的可读事件，读取字节流，编码后再回应给客户端。
                socket_fd = events[i].data.fd;
                printf("get event on socket fd == %d\n", socket_fd);
                while (1) {
                    char buf[512];
                    if ((n = read(socket_fd, buf, sizeof(buf))) < 0 ) {
                        if (errno != EAGAIN) {
                            error(1, error, "read error");
                            close(socket_fd);
                        }
                        break;
                    } else if(n == 0) {
                        close(socket_fd);
                        break;
                    } else {
                        for (i = 0; i<n; ++i) {
                            buf[i] = rot13_char(buf[i]);
                        }
                        if (write(socket_fd, buf, n) < 0) {
                            error(1, errno, "write error");
                        }
                    }
                }
            }
        }
    }
    free(events);
    close(listen_fd);
    return 0;
}