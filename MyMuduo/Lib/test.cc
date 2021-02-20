#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
//#define gettid() syscall(__NR_gettid)
#include <sys/types.h>
pid_t gettid()
{
    printf("ok\n");
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

__thread int s_nTid = 0;

void* ThreadFun(void* arg)
{
    printf("%d_1\n", (int)gettid());
    s_nTid = (int)gettid();
    printf("%d_2\n", (int)gettid());
    sleep(4);
    printf("tid=%d\n", s_nTid);
}

int main()
{
    pthread_t _arr[5];
    for(int i = 0; i < 5; i++)
    {
        pthread_create(
            &_arr[i], 
            NULL, 
            ThreadFun, 
            NULL);
    }

    for(int i = 0; i < 5; i++)
    {
        pthread_join(_arr[i], NULL);
    }
        
    printf("main_%d_%d\n", (int)gettid(), (int)getpid());
}
