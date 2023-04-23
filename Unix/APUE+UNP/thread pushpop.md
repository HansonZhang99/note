# thread push/pop

```cpp
/*两个线程都调用了，但是却只调用了第二个线程的清理处理程序，所以如果线程是通过从它的启动历程中返回而终止的话，那么它的清理处理程序就不会被调用，还要注意清理程序是按照与它们安装时相反的顺序被调用的。从代码输出也可以看到先执行的thread 2 second handler后执行的thread 2 first handler。
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


void cleanup(void *arg)
{
    printf("cleanup:%s\n",(char*)arg);
}
void *thr_fn1(void *arg)
{
    printf("thread 1 start\n");
    pthread_cleanup_push(cleanup,"thread 1 first handler");
    pthread_cleanup_push(cleanup,"thread 1 second handler");
    printf("thread 1 push complete\n");
    if(arg)
        return ((void *)1);
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);
    return ((void *)1);
}
void *thr_fn2(void *arg)
{
    printf("thread 2 start\n");
    pthread_cleanup_push(cleanup,"thread 2 first handler");
    pthread_cleanup_push(cleanup,"thread 2 second handler");
    printf("thread 2 push complete\n");
    if(arg)
        pthread_exit((void *)2);
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);
    pthread_exit((void *)2);
}
int main()
{
    int err;
    pthread_t tid1,tid2;
    void *tret;
    err = pthread_create(&tid1,NULL,thr_fn1,(void *)1);
    if(err != 0)
    {
        fprintf(stderr,"thread create 1 is error\n");
        return -1;
    }
    err = pthread_create(&tid2,NULL,thr_fn2,(void *)1);
    if(err != 0)
    {
        fprintf(stderr,"thread create 2 is error\n");
        return -2;
    }
    err = pthread_join(tid1,&tret);
    if(err != 0)
    {
        fprintf(stderr,"can't join with thread 1\n");
        return -2;
    }


    //pthread_cancel(tid1);
    printf("thread 1 exit code %d\n",tret);
    err = pthread_join(tid2,&tret);
    if(err != 0)
    {
        fprintf(stderr,"can't join with thread 2\n");
        return -2;
    }
    printf("thread 2 exit code %d\n",tret);
    return 0;
}
```

