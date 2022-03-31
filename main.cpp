#include <cstring>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<signal.h>
#include <libgen.h>
#include"http_conn.h"
#include"locker.h"
#include"threadpool.h"
#include"sql_conn.h"

#define MAX_FD 65535 //最大文件描述符个数
#define MAX_EVENT_NUMBER 10000 //最大的监听事件数量


//添加信号捕捉，需要捕捉SIGPIPE信号，客户端向服务端发送了消息后，关闭客户端，服务端返回消息时会收到SIGPIPE信号
//方案是捕捉SIGPIPE信号，然后在处理函数里什么都不做
void addsig(int sig, void(handler)(int)){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

//添加文件描述符到epoll中
extern void addfd(int epollfd, int fd, bool one_shot);

//从epoll中删除文件描述符
extern void removefd(int epollfd, int fd);

//修改epoll文件描述符
extern void modfd(int epollfd, int fd, int ev);


int main(int argc, char* argv[]){
    //如果传递的参数小于等于1这说明没有传递参数
    if(argc <= 1){
        printf("按照如下格式运行：%s prot_number\n", basename(argv[0]));
        exit(-1);
    }

    //获取端口号
    int port = atoi(argv[1]);

    //对SIGPIPE进行处理，捕捉到SIGPIPE什么也不做
    addsig(SIGPIPE, SIG_IGN);
    //创建线程池
    threadpool<http_conn> * pool = NULL;
    //创建数据库连接池
    
    try{
        pool = new threadpool<http_conn>;
    }catch(...){
        exit(-1);
    }

    //创建数组保存所有客户端信息
    http_conn * users = new http_conn [MAX_FD];

    //监听套接字创建(TCP,IPV4)
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);

    //设置端口复用，运行服务端重用本地端口(一定在绑定之前)
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    //绑定端口
    struct sockaddr_in address;
    //ipv4协议
    address.sin_family = AF_INET;
    //端口号转换为网络字节序
    address.sin_port = htons(port);
    //主机的ip地址
    address.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(listenfd, (struct sockaddr*)&address,sizeof(address));
    if(ret == -1){
        perror("bind:");
        exit(-1);
    }

    //监听
    listen(listenfd, 5);

    //创建epoll对象，事件的数组，添加监听的文件描述符
    epoll_event events[MAX_EVENT_NUMBER];
    //传任何非0值
    int epollfd = epoll_create(5);
    //添加监听的文件描述符到epoll对象中,监听文件描述符不需要oneshot
    addfd(epollfd, listenfd, false);
    //初始化静态成员，使所有的连接共用一个epollfd
    http_conn::m_epollfd = epollfd;
    while(true){
        //检测有几个epoll事件
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        //如果返回值比0小，或者不是中断，证明epoll调用失败
        if((num<0)&&(errno != EINTR)){
            printf("epoll failuer\n");
            break;
        }

        //循环遍历事件数组
        for(int i=0;i<num;++i){
            int sockfd = events[i].data.fd;
            if(sockfd==listenfd){
                //描述符等于监听的文件描述符，证明有客户端连接进来
                struct sockaddr_in client_address;
                socklen_t client_addrlen = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address,&client_addrlen);

                if(http_conn::m_user_count >= MAX_FD){
                    //说明目前连接数满了，告诉客户端服务器正忙
                    //给客户端写信息
                    close(connfd);
                    continue;
                }
                //将客户数据初始化放入user数组
                users[connfd].init(connfd, client_address);
            }else if(events[i].events & (EPOLLRDHUP|EPOLLHUP|EPOLLERR)){
                //对方异常断开或者错误事件发生
                users[sockfd].close_conn();
            }else if(events[i].events & EPOLLIN){
                //一次性把所有数据读出来
                if(users[sockfd].read()){
                    pool->append_task(users+sockfd);
                }else {
                    users[sockfd].close_conn();
                }
            }else if(events[i].events & EPOLLOUT){
                //一次性写完所有数据
                if(!users[sockfd].write()){
                    users[sockfd].close_conn();
                }
            }
        }
    }
    close(epollfd);
    close(listenfd);
    delete [] users;
    delete pool;


    return 0;
}