#ifndef __HTTP_CONN_H__
#define __HTTP_CONN_H__

#include<sys/epoll.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/types.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<errno.h>
#include<stdarg.h>
#include<sys/uio.h>
#include"locker.h"


class http_conn{
public:
    static int m_epollfd;//所有的socket事件都被注册到同一个epoll事件中。
    static int m_user_count; //连接的用户数量。

    http_conn(){}
    ~http_conn(){}
    //处理客户端的请求，即解析HTTP请求报文，并响应客户端
    void process();
    void init(int sockfd, const sockaddr_in & addr); //初始化新接收的连接
    void close_conn();//关闭连接
    bool read(); //非阻塞读
    bool write(); //非阻塞写
private:
    int m_sockfd; //该客户端连接的socket
    sockaddr_in m_address; //通信的socket地址
};











#endif