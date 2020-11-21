#include "friendmodel.hpp"
#include "CommonConnectionPool.h"

//添加好友关系
void FriendModel::insert(int userId, int friendId)
{
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend(userid,friendid) values(%d,%d)", userId, friendId);
    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    //3： 执行增删改操作
    sp->update(sql);
}
//返回用户好友列表, user和friend表的联合查询
vector<User> FriendModel::query(int userId)
{
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid=a.id where b.userid=%d", userId);
    vector<User> vec;
    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    //3： 执行查询操作
    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setState(row[2]);
            vec.push_back(user);
        }
        mysql_free_result(res);
        return vec;
    }
    return vec;
}