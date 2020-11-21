#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
#include <ctime>
using namespace std;

// 数据库操作类
class MySQL
{
public:
    // 初始化数据库连接
    MySQL();
    // 释放数据库连接资源
    ~MySQL();
    // 连接数据库
    bool connect(std::string ip,
                 unsigned short port,
                 std::string user,
                 std::string password,
                 std::string dbname);
    // 增，删，改操作
    bool update(string sql);
    // 查询操作
    MYSQL_RES *query(string sql);
    // 获取连接
    MYSQL *getConnection();
    //刷新连接的起始空闲时间点
    void refreshAliveTime();
    //返回存活时间
    clock_t getAliveTime() const;

private:
    MYSQL *_conn;      //表示和MYSQL Server的一条连接
    clock_t alivetime; //记录进入空闲状态后的起始存活时间
};

#endif //DB_H