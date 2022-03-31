#include"sql_conn.h"

//初始化静态变量
connpool * connpool::m_conn_pool = NULL;

//获取连接对象
connpool * connpool::get_instance(){
    //单例
    if(m_conn_pool==NULL){
        m_conn_pool = new connpool("tcp://127.0.0.1:3306", "root", "123456", 20);
    }
    return m_conn_pool;
}

//构造函数
connpool::connpool(std::string url, std::string user, std::string password, int max_conn_size){
    this->max_conn_size = max_conn_size;
    this->cur_size = 0;
    this->url = url;
    this->username = user;
    this->password = password;
    try{
        this->m_driver = sql::mysql::get_driver_instance();
    }
    catch(sql::SQLException& e){
        perror("get driver error.\n");
    }
    catch(...){
        perror("[ConnPool] run time error.\n");
    }
    //初始化连接时给池中建立一定数量的数据库连接
    this->init_connection(max_conn_size/2);
}

//初始化数据库连接池
void connpool::init_connection(int i_initial){
    sql::Connection* conn;
    conn_locker.lock();
    for (int i = 0; i < i_initial; i++)
    {
        conn = this->create_connection();
        if(conn){
            conn_list.push_back(conn);
            ++(this->cur_size);
        }
        else{
            perror("Init connection error\n");
        }
    }
    conn_locker.unlock();
}

//创建并返回一个连接
sql::Connection * connpool::create_connection(){
    sql::Connection *conn;
    try{
        //建立连接
        conn = m_driver->connect(this->url, this->username,this->password);
        return conn;
    }
    catch(sql::SQLException& e){
        perror("create connection error.\n");
        return NULL;
    }
    catch(...){
        perror("[CreateConnection] run time error.\n");
        return NULL;
    }
}

//从连接池中获取一个连接
sql::Connection * connpool::get_connection(){
    sql::Connection *con;
    conn_locker.lock();

    //如果连接池中还有连接
    if(conn_list.size()>0){
        con = conn_list.front();
        conn_list.pop_front();
        //如果连接已经关闭，删除后重新创建一个
        if(con->isClosed()){
            delete con;
            con = this->create_connection();
            if(con==NULL){
                //说明创建出错
                --cur_size;
            }
        }
        conn_locker.unlock();
        return con;
    }
    //如果池中没有连接
    else{
        //如果当前连接数小于最大连接数，创建新连接
        if(cur_size<max_conn_size){
            con = this->create_connection();
            if(con){
                ++cur_size;
                conn_locker.unlock();
                return con;
            }
            else{
                conn_locker.unlock();
                return NULL;
            }
        }
        //当前接连数大于最大连接数
        else{
            perror("[get_connection] connections reach max number.\n");
            conn_locker.unlock();
            return NULL;
        }
    }
}

//释放数据库连接，将连接放回连接池
void connpool::release_connection(sql::Connection * conn){
    if(conn){
        conn_locker.lock();
        conn_list.push_back(conn);
        conn_locker.unlock();
    }
}

//析构
connpool::~connpool(){
    this->destory_connpool();
}

//销毁数据库连接池之前需要销毁单个连接
void connpool::destory_connpool(){
    std::list<sql::Connection*>::iterator itcon;
    conn_locker.lock();
    for(itcon=conn_list.begin(); itcon!=conn_list.end();++itcon){
        this->destory_connection(*itcon);
    }
    cur_size=0;
    conn_list.clear();
    conn_locker.unlock(); 
}

//销毁单个连接
void connpool::destory_connection(sql::Connection * conn){
    if(conn){
        try{
            //关闭连接
            conn->close();
        }
        catch(sql::SQLException& e){
            perror(e.what());
        }
        catch(...){
             perror("意料之外的错误\n");
        }
        delete conn;
    }
}