#include "usermodel.hpp"
#include "db.h"
#include <iostream>
#include "CommonConnectionPool.h"
using namespace std;

//User表的插入
bool UserModel::insert(User &user)
{
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name,password,state) values('%s','%s','%s')",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());

    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    //3： 执行增删改操作
    if (sp->update(sql))
    {
        //获取插入成功的用户数据生成的主键id
        user.setId(mysql_insert_id(sp->getConnection()));
        return true;
    }

    // MySQL mysql;
    // //2: 连接数据库
    // if (mysql.connect())
    // {
    //     //3： 执行增删改查操作
    //     if (mysql.update(sql))
    //     {
    //         //获取插入成功的用户数据生成的主键id
    //         user.setId(mysql_insert_id(mysql.getConnection()));
    //         return true;
    //     }
    // }

    return false;
}

//User表根据id查询
User UserModel::query(int id)
{
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id=%d", id);
    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    //3： 执行查询操作
    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr)
        {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPassword(row[2]);
            user.setState(row[3]);
            mysql_free_result(res);
            return user;
        }
    }
    return User();
}

//更新用户的状态信息
bool UserModel::updateState(User user)
{
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state='%s' where id=%d", user.getState().c_str(), user.getId());
    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    //3： 执行增删改操作
    if (sp->update(sql))
    {
        return true;
    }
    return false;
}

//重置用户的状态信息
void UserModel::resetState()
{
    //1: 组装sql语句
    char sql[1024] = "update user set state='offline' where state='online'";
    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    //3： 执行增删改操作
    sp->update(sql);
}