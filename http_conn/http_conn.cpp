#include"http_conn.h"

//设置文件描述符非阻塞
void setnonblocking(int fd){
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
}


//添加文件描述符到epoll中
void addfd(int epollfd, int fd, bool one_shot){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP;
    //防止多次触发多个线程处理一个socket，在判断使用后立即重置
    if(one_shot){
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    //设置文件描述符非阻塞
    setnonblocking(fd);
}


//从epoll中删除文件描述符
void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//修改epoll文件描述符，最主要的是重置EPOLLONESHOT事件，确保下一次可读时EPOLLIN可以被触发。
void modfd(int epollfd, int fd, int ev){
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

//初始化连接
void http_conn::init(int sockfd, const sockaddr_in & addr){
    m_sockfd = sockfd;
    m_address = addr;

    //设置端口复用
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    //添加到epoll对象中,需要oneshot
    addfd(m_epollfd, m_sockfd, true);
    //总用户数加1
    m_user_count++;
}

//关闭连接
void http_conn::close_conn(){
    if(m_sockfd != -1){
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        //用户总数减一
        m_user_count--;
    }
}

//一次性读完数据
bool http_conn::read(){
    printf("一次性读完数据\n");
    return true;
}

//一次性写完数据
bool http_conn::write(){
    printf("一次性写完数据\n");
    return true;
}

//由线程池中的工作线程调用，处理http请求的入口函数
void http_conn::process(){
    //解析http请求
    printf("parse request, create response\n");
    //生成响应
}