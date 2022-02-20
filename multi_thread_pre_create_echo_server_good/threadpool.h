#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 *线程体结构体
 */
typedef struct runner {
  void *(*callback)(void *arg);
  void *arg;
  struct runner *next;
} thread_runner;

/*
 *线程池结构
 */
typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  //线程池所有等待队列的头指针
  thread_runner *runner_head;
  //线程池所有等待队列的尾指针
  thread_runner *runner_tail;
  //所有线程
  pthread_t *threads;
  //线程池能活动的线程数量
  int max_thread_size;
  //线程池是否销毁
  int shutdown;
} thread_pool;

/*
 *线程执行任务
 */
void run(void *arg);

/*
 *初始化线程池
参数
pool：指向线程池结构有效地址的指针
max_thread_size：初始化线程池的大小
 */
void thread_pool_init(thread_pool *pool, int max_thread_size);

/*
 *向线程池加入任务
 参数
 pool：指向线程池结构有效地址的指针
 callback：回调函数
 arg;回调函数参数
 */
void threadpool_add_runner(thread_pool *pool, void *(*callback)(void *arg),
                           void *arg);

/*
 *销毁线程池
 */
void thread_destroy(thread_pool **ppool);

#endif