#ifndef __SQL_CONN_H__
#define __SQL_CONN_H__

#include<stdexcept>
#include<exception>
#include<stdio.h>
#include<string>
#include<mysql_connection.h>
#include<mysql_driver.h>
#include<cppconn/driver.h>
#include<cppconn/statement.h>
#include<cppconn/prepared_statement.h>
#include<cppconn/exception.h>
#include<cppconn/resultset.h>
#include<cppconn/connection.h>
#include<pthread.h>
#include<list>
#include"locker.h"

//数据库连接池
class connpool{
public:
    ~connpool();
    //获取数据库连接
    sql::Connection* get_connection();
    //将数据库连接放回到连接池
    void release_connection(sql::Connection* conn);
    //获取数据库连接池对象，只需要一个设置为静态
    static connpool* get_instance();

private:
    //已建立连接数量
    int cur_size;
    //最大连接数
    int max_conn_size;
    //连接数据库所需
    std::string username;
    std::string password;
    std::string url;
    //连接池容器环形链表
    std::list<sql::Connection*> conn_list;
    //锁
    locker conn_locker;
    static connpool* m_conn_pool;
    sql::Driver* m_driver;

    //创建连接
    sql::Connection* create_connection();
    //初始化数据库连接池
    void init_connection(int i_initial);
    //销毁数据库连接对象
    void destory_connection(sql::Connection* conn);
    //销毁数据库连接池
    void destory_connpool();

    //构造方法
    connpool(std::string url, std::string user,std::string password,int max_conn_size);    
};

#endif