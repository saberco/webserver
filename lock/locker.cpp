#include"locker.h"

//锁类实现
locker::locker(){
    //init返回0初始化成功，返回非0为错误码
    if(pthread_mutex_init(&m_mutex, NULL)!=0){
        //抛出异常
        throw std::exception();
    }
}

locker::~locker(){
    pthread_mutex_destroy(&m_mutex);
}

bool locker::lock(){
    //不等于0则上锁失败
    return pthread_mutex_lock(&m_mutex) == 0;
}

bool locker::unlock(){
    return pthread_mutex_unlock(&m_mutex) == 0;
}

pthread_mutex_t * locker::get(){
    return &m_mutex;
}

//条件变量类实现
mycond::mycond(){
    if(pthread_cond_init(&m_cond,NULL) != 0){
        throw std::exception();
    }
}

mycond::~mycond(){
    pthread_cond_destroy(&m_cond);
}

bool mycond::wait_cond(pthread_mutex_t * mutex){
    return pthread_cond_wait(&m_cond, mutex) == 0;
}

bool mycond::timed_wait_cond(pthread_mutex_t * mutex, timespec t){
    return pthread_cond_timedwait(&m_cond, mutex, &t) == 0;
}

bool mycond::signal_cond(){
    return pthread_cond_signal(&m_cond) == 0;
}

bool mycond::broad_cast_cond(){
    return pthread_cond_broadcast(&m_cond)==0;
}

sem::sem(){
    if(sem_init(&m_sem, 0, 0)!=0){
        throw std::exception();
    }
}

sem::sem(int num){
    //中间参数一般设0为多线程共享，num为可用资源数
    if(sem_init(&m_sem, 0, num)!=0){
        throw std::exception();
    }
}

sem::~sem(){
    sem_destroy(&m_sem);
}

bool sem::wait_sem(){
    return sem_wait(&m_sem) == 0;
}

bool sem::post(){
    return sem_post(&m_sem);
}