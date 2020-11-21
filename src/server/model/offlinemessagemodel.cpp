#include "offlinemessagemodel.hpp"
#include "CommonConnectionPool.h"
#include "db.h"

//存储用户的离线消息
void OfflineMsgModel::insert(int userId, string msg)
{
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage(userid,message) values(%d,'%s')", userId, msg.c_str());

    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    //3： 执行增删改操作
    sp->update(sql);
}
//删除用户的离线消息
void OfflineMsgModel::remove(int userId)
{
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userId);

    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    //3： 执行增删改操作
    sp->update(sql);
}
//查询用户的离线消息
vector<string> OfflineMsgModel::query(int userId)
{
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid=%d", userId);
    vector<string> vec;
    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    //3： 执行查询操作
    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        //把userid用户所有离线消息放入vec中返回给该用户
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            vec.push_back(row[0]);
        }
        mysql_free_result(res);
        return vec;
    }
    return vec;
}