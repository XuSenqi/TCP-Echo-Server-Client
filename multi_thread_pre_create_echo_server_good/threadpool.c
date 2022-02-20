#include "threadpool.h"

#define DEBUG

/*
 *初始化线程池
 */
void thread_pool_init(thread_pool* pool,int max_thread_size)
{
	//初始化互斥量
	pthread_mutex_init(&(pool->mutex),NULL);
	//初始化条件变量
	pthread_cond_init(&(pool->cond),NULL);
	pool->runner_head = NULL;
	pool->runner_tail = NULL;
	pool->max_thread_size = max_thread_size;
	pool->shutdown = 0;

	//创建所有分离态线程
	pool->threads = (pthread_t*)malloc(max_thread_size*sizeof(pthread_t));
    int i;
	for(i = 0;i<max_thread_size;i++)
	{
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
		pthread_create(&(pool->threads[i]),&attr,(void*)run,(void*)pool);
	}
#ifdef DEBUG
	printf("thread_pool_create %d detached thread\n",max_thread_size);
#endif
}

/*
 *线程体
 */
void run(void* arg)
{
	thread_pool* pool = (thread_pool*)arg;
	while(1)
	{
		//加锁
		pthread_mutex_lock(&(pool->mutex));
#ifdef DEBUG
		printf("run->lock\n");
#endif
		//如果等待队列为0并且线程池未销毁，则阻塞
		while(pool->runner_head == NULL&&!pool->shutdown)
		{
			pthread_cond_wait(&(pool->cond),&(pool->mutex));
		}

		//如果线程池已经销毁
		if(pool->shutdown)
		{
			//解锁
			pthread_mutex_unlock(&(pool->mutex));
#ifdef DEBUG
			printf("run->lock and thread exit\n");
#endif
			//线程推出
			pthread_exit(NULL);	
		}

		//取得任务队列中的第一个任务
		thread_runner* runner = pool->runner_head;
		pool->runner_head = runner->next;
		pthread_mutex_unlock(&(pool->mutex));
#ifdef DEBUG
		printf("run->unlock\n");
#endif
		//调用回调（执行任务）
		(runner->callback)(runner->arg);
		free(runner);
		runner = NULL;
#ifdef DEBUG
		printf("run->runned and free runner\n");
#endif
	}
	pthread_exit(NULL);
}

/*
 *向线程池添加任务
 */
void threadpool_add_runner(thread_pool* pool,void*(*callback)(void* arg),void* arg)
{
	//构造一个新任务
	thread_runner* newrunner = (thread_runner*)malloc(sizeof(thread_runner));
	newrunner->callback = callback;
	newrunner->arg = arg;
	newrunner->next = NULL;

	//上锁，把新任务插入任务队列中
	pthread_mutex_lock(&(pool->mutex));
#ifdef DEBUG
	printf("threadpool_add_runner->locked\n");
#endif
	if(pool->runner_head != NULL)
	{
		pool->runner_tail->next = newrunner;
		pool->runner_tail = newrunner;
	}
	else
	{
		pool->runner_head = newrunner;
		pool->runner_tail = newrunner;
	}
	//解锁
	pthread_mutex_unlock(&(pool->mutex));
#ifdef DEBUG
	printf("threadpool_add_runner->unlocked\n");
#endif
	//唤醒等待队列
	pthread_cond_signal(&(pool->cond));
#ifdef DEBUG
	printf("threadpool_add_runner->add a runner and wakeip a waiting thread\n");
#endif
}

/*
*销毁线程
 */
void thread_destroy(thread_pool** ppool)
{
	thread_pool* pool = *ppool;
	//防止二次销毁
	if(!pool->shutdown)
	{
		pool->shutdown = 1;
		//唤醒所有等待队列，线程池要销毁了
		pthread_cond_broadcast(&(pool->cond));
		sleep(1);
#ifdef DEBUG 
		printf("threadpool_destroy->wakeup all waiting thread\n");
#endif
		//回收线程
		free(pool->threads);
		//销毁等待队列
		thread_runner *head = NULL;
		while(pool->runner_head != NULL)
		{
			head = pool->runner_head;
			pool->runner_head = pool->runner_head->next;
			free(head);
		}
#ifdef DEBUG
		printf("threadpool_destroy->all runner freed\n");
#endif
		//销毁：条件变量和互斥量
		pthread_mutex_destroy(&(pool->mutex));
		pthread_cond_destroy(&(pool->cond));
#ifdef DEBUG
		printf("threadpool_destroy->mutex and cond destoryed\n");
#endif
		free(pool);
		(*ppool) = NULL;
#ifdef DEBUG
		printf("threadpool_destroy->pool freed\n");
#endif
	}
}