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
#include<cstring>
#include<string.h>
#include<boost/format.hpp>
#include"locker.h"
#include"sql_conn.h"

class http_conn{
public:
    static int m_epollfd;//所有的socket事件都被注册到同一个epoll事件中。
    static int m_user_count; //连接的用户数量。
    static const int FILENAME_LEN = 200;        // 文件名的最大长度
    //读写缓冲大小
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 2048;
    //HTTP请求方法
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};
    /*解析客户端请求时，主状态机的状态
    CHECK_STATE_REQUESTLINE：解析请求行
    CHECK_STATE_HEAD：解析请求头
    CHECK_STATE_CONTENT：解析请求体
    */
    enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEAD, CHECK_STATE_CONTENT};
    /*从状态机的三种可能状态
    LINE_OK：读取到一个完整的行
    LINE_BAD：行出错
    LINE_OPEN：行数据不完整
    */
    enum LINE_STATE {LINE_OK = 0, LINE_BAD, LINE_OPEN};
    /*服务器处理HTTP请求的可能结果，报文解析的结果
        NO_REQUEST              ：请求不完整，需要继续读取客户数据
        GET_REQUEST             ：获得了一个完整的客户请求
        BAD_REQUEST             ：表示客户请求语法错误
        NO_RESOURCE             ：服务器没有资源
        FORBIDDEN_REQUEST       ：客户对服务器没有足够的访问权限
        FILE_REQUEST            ：文件请求，获取文件成功
        INTERNAL_ERROR          ：表示服务器内部错误
        CLOSED_CONNECTION       ：表示客户端已经关闭连接
    */
    enum HTTP_CODE {NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST , INTERNAL_ERROR, CLOSED_CONNECTION };
    
    enum POST_ACT {LOGIN=0, REGISTER};
    
    connpool* connPool;


    http_conn(){connPool = connpool::get_instance();}
    ~http_conn(){}
    //处理客户端的请求，即解析HTTP请求报文，并响应客户端
    void process();
    void init(int sockfd, const sockaddr_in & addr); //初始化新接收的连接
    void close_conn();//关闭连接
    bool read(); //非阻塞读
    bool write(); //非阻塞写

    //主状态机，解析每一部分
    HTTP_CODE process_read();   //解析HTTP请求
    bool process_write( HTTP_CODE ret );    // 填充HTTP应答

    HTTP_CODE parse_request_line(char * text);     //解析请求首行
    HTTP_CODE parse_headers(char * text);     //解析请求头
    HTTP_CODE parse_content(char * text);     //解析请求体

    //从状态机，解析每一行
    LINE_STATE parse_line();    //解析具体的一行
    HTTP_CODE do_request();

    // 这一组函数被process_write调用以填充HTTP应答。
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_content_type();
    bool add_status_line( int status, const char* title );
    void add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();


private:
    int m_sockfd; //该客户端连接的socket
    sockaddr_in m_address; //通信的socket地址

    char m_read_buf[READ_BUFFER_SIZE];//读缓冲区
    int m_read_idx; //标识读缓冲区已经读入的客户端数据的最后一个字节的下一个位置

    int m_checked_index;//当前正在解析的字符在读缓冲区的位置
    int m_start_line;   //当前解析行的在数组中的起始位置

    //POST请求相关
    char * m_first;
    char * m_mail;
    char * m_username;
    char * m_password;


    char * m_url;//请求目标文件的文件名
    char * m_version; //HTTP协议版本
    char m_real_file[FILENAME_LEN];// 客户请求的目标文件的完整路径，其内容等于 doc_root + m_url, doc_root是网站根目录
    char * m_host;  //主机名
    bool m_liner;   //判断http是否需要保持连接
    long int m_content_length;

    char m_write_buf[ WRITE_BUFFER_SIZE ];  // 写缓冲区
    int m_write_idx;                        // 写缓冲区中待发送的字节数
    char* m_file_address;                   // 客户请求的目标文件被mmap到内存中的起始位置
    struct stat m_file_stat;                // 目标文件的状态。通过它可以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息
    struct iovec m_iv[2];                   // 采用writev来执行写操作，所以定义下面两个成员，其中m_iv_count表示被写内存块的数量。
    int m_iv_count;

    int bytes_to_send;              // 将要发送的数据的字节数
    int bytes_have_send;            // 已经发送的字节数

    METHOD m_method;    //请求方法
    CHECK_STATE m_check_state; //主状态机当前状态
    POST_ACT m_post_act;//是登录还是注册

    //数据库相关
    sql::Connection* m_conn_sql;
    sql::Statement * m_state_sql;
    sql::ResultSet * m_result_sql;
    sql::PreparedStatement * m_prep_state;

private:
    void init();    //初始化其余的数据
    //从一行的起始位置获取新的数据，遇到\n\r结束
    char * get_line(){
        return m_read_buf + m_start_line;
    }
};

#endif