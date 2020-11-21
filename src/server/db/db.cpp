#include "db.h"
#include <muduo/base/Logging.h>
using namespace muduo;

// 数据库配置信息
// static string server = "127.0.0.1";
// static string user = "root";
// static string password = "dr5262000";
// static string dbname = "chat";

// 初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}
// 连接数据库
bool MySQL::connect(std::string ip, unsigned short port, std::string user, std::string password, std::string dbname)
{
    //MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(), password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    MYSQL* p = mysql_real_connect(_conn, ip.c_str(), user.c_str(), 
		password.c_str(), dbname.c_str(), port, nullptr, 0);
    if (p != nullptr)
    {
        //c和c++代码默认的字符是ASCLL，如果不设置，从Mysql获取数据的中文显示“?”
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!!!";
    }
    else
    {
        LOG_INFO << "connect mysql fail!!!";
    }
    return p;
}
// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败!";
        return false;
    }
    return true;
}
// 查询操作
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}
//获取连接
MYSQL *MySQL::getConnection()
{
    return _conn;
}
//刷新连接的起始空闲时间点
void MySQL::refreshAliveTime()
{
    alivetime = clock();
}
//返回存活时间
clock_t MySQL::getAliveTime() const
{
    return clock() - alivetime;
}