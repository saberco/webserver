#include"sql_conn.h"

connpool* connPoll = connpool::get_instance();

int main(int argc, char* argv[])
{
    sql::Connection *con;
    sql::Statement *state;
    sql::ResultSet *result;
    
    // 从连接池中获取连接
    con = connPoll->get_connection();
 
    for (int i = 0; i < 100; i++)
    {
        state = con->createStatement();
        state->execute("use test");
 
        // 查询数据库
        result = state->executeQuery("select * from users");
        // 打印数据库查询结果
        std::cout << "================================" << std::endl;
        while (result->next())
        {
            std::string strOccupation = result->getString("name");
            std::string strCamp = result->getString("pwd");
            std::cout <<  strOccupation << " , " << strCamp << std::endl;
        }
        std::cout << "i is: " << i << std::endl;
        std::cout << "================================" << std::endl;
    }
 
    delete result;
    delete state;
    connPoll->release_connection(con);
 
    return 0;
}