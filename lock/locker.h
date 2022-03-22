#ifndef __LOCKER_H__
#define __LOCKER_H__

#include<pthread.h>
#include<exception>
#include<semaphore.h>

//线程同步机制封装类
//互斥锁类
class locker{
public:
    //初始化锁
    locker();
    //析构锁
    ~locker();
    //上锁
    bool lock();
    //解锁
    bool unlock();
    //获取互斥锁
    pthread_mutex_t * get();
private:
    pthread_mutex_t m_mutex;
};

//条件变量类
class mycond{
public:
    mycond();
    ~mycond();

    bool wait_cond(pthread_mutex_t* mutex);
    bool timed_wait_cond(pthread_mutex_t* mutex, struct timespec t); 
    bool signal_cond();
    bool broad_cast_cond();
private:
    pthread_cond_t m_cond;
};

//信号量类
class sem{
private:
    sem_t m_sem;
public:
    sem();
    sem(int num);
    ~sem();
    //等待信号量，相当于P操作，申请资源
    bool wait_sem();
    //增加信号量，相当于V操作，释放资源
    bool post();
};





#endif