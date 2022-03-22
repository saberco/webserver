#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

//线程池类

#include<list>
#include<exception>
#include<pthread.h>
#include<cstdio>
#include"locker.h"
//线程池模板类，为了代码的复用性，适应变化的任务类型
template<typename T>
class threadpool{
public:
    //构造函数
    threadpool(int thread_number = 8, int max_requests=10000);
    //析构函数
    ~threadpool();
    //向工作队列添加任务
    bool append_task(T* request);

private:
    static void * worker(void * arg);
    void run();

private:
    //线程池线程的数量
    int m_thread_number;

    //装线程的容器，这里是线程池数组（动态数组），大小为m_thread_number
    pthread_t * m_threads;

    //任务队列最大允许等待数量
    int m_max_requests;

    //任务队列，这里用双向环形链表
    std::list< T* > m_work_queue;

    //互斥锁
    locker m_queue_locker;

    //信号量来判断是否有任务需要处理
    sem m_queuestat;

    //是否结束线程
    bool m_stop;
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) : m_thread_number(thread_number),m_max_requests(max_requests), m_stop(false),m_threads(NULL){
    if((thread_number<=0)||(max_requests<=0)){
        throw std::exception();
    }
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads){
        throw std::exception();
    }
    //创建thread_number个线程，并设置为detach线程脱离，使用完自己释放资源
    for(int i=0;i<m_thread_number;++i){
        printf("create the %dth thread\n", i);
        //c++中线程工作函数一定要是静态函数,主要是为了告诉线程起始地址和类中编译不要产生this指针（单例）。
        //如果创建失败，抛出异常，delete动态数组
        //由于work是静态成员函数不能访问非静态成员变量，所以需要将this指针作为参数传递给work
        if(pthread_create(m_threads+i, NULL, worker, this)!=0){
            delete [] m_threads;
            throw std::exception();
        }
        //创建成功则设置线程分离
        if(pthread_detach(m_threads[i])){
            delete [] m_threads;
            throw std::exception();
        }
    }
}

//析构函数
template<typename T>
threadpool<T>::~threadpool(){
    delete [] m_threads;
    //停止线程
    m_stop = true;
}

//添加任务到任务队列
template<typename T>
bool threadpool<T>::append_task(T * request){

    m_queue_locker.lock();
    //当工作队列中的任务数量多于最大的工作数，则不能继续添加，解锁并返回false;
    if(m_work_queue.size() > m_max_requests){
        m_queue_locker.unlock();
        return false;
    }
    //没有则可以添加
    m_work_queue.push_back(request);
    m_queue_locker.unlock();
    //增加信号量，因为需要唤醒睡眠的线程，使得线程从sem_wait状态退出进入工作状态，一个任务可以唤醒一个线程。
    m_queuestat.post();
    return true;
}
//线程工作函数
template<typename T>
void * threadpool<T>::worker(void * arg){
    threadpool* pool = (threadpool*) arg;
    pool->run();
    return pool;
}

//线程具体的工作，单独写为了避免大量的pool->
template<typename T>
void threadpool<T>::run(){
    while(!m_stop){
        //如果工作队列没有工作，则信号量为0，线程沉睡在这直到任务进来，信号量被post
        m_queuestat.wait_sem();
        //走到下一步说明有任务进来
        //上锁
        m_queue_locker.lock();
        //再次检测工作队列是否有任务
        if(m_work_queue.empty()){
            //没有解锁继续沉睡
            m_queue_locker.unlock();
            continue;
        }
        //到这说明肯定有任务
        //获取第一个任务
        T* request = m_work_queue.front();
        m_work_queue.pop_front();
        m_queue_locker.unlock();
        //如果没有获取到
        if(!request){
            continue;
        }
        //如果获取到了
        request->process();
    }
}















#endif