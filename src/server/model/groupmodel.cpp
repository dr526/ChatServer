#include "groupmodel.hpp"
#include "CommonConnectionPool.h"
#include "db.h"

//创建群组
bool GroupModel::createGroup(Group &group)
{
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname,groupdesc) values('%s','%s')", group.getName().c_str(), group.getDesc().c_str());
    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    //3： 执行增删改操作
    if (sp->update(sql))
    {
        //获取插入成功的数据生成的主键id
        group.setId(mysql_insert_id(sp->getConnection()));
        return true;
    }
    return false;
}

//加入群组
void GroupModel::addGroup(int userId, int groupId, string role)
{
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser(groupid,userid,grouprole) values(%d,%d,'%s')", groupId, userId, role.c_str());
    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    //3： 执行增删改操作
    sp->update(sql);
}

//查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userId)
{
    /*
    1. 先根据userId在groupuser表中查询该用户所属群组信息
    2. 再根据群组信息，查询该群组的所有用户的userId，并且和user表进行多表联合查询，查出用户的详细信息
    */
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join groupuser b on a.id=b.groupid where userid=%d", userId);
    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    vector<Group> groupVec;
    //3： 执行查询操作
    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        //查出userId用户所属的所有群组信息
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            Group group;
            group.setId(atoi(row[0]));
            group.setName(row[1]);
            group.setDesc(row[2]);
            groupVec.push_back(group);
        }
        mysql_free_result(res);
    }
    //查询群组的用户信息
    for (Group &group : groupVec)
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join groupuser b on b.userid=a.id where b.groupid=%d", group.getId());
        //执行查询操作
        MYSQL_RES *res = sp->query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            //查出userId用户所属的所有群组信息
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}
//根据指定的groupId查询群组用户id列表，除userId本身，主要用户群聊业务给群组其他成员群发消息
vector<int> GroupModel::queryGroupUsers(int userId, int groupId)
{
    //1: 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid=%d and userid!=%d", groupId, userId);
    //2: 通过数据库连接池获取连接
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    shared_ptr<MySQL> sp = cp->getConnection();
    vector<int> idVec;
    //3： 执行查询操作
    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        //查出userId用户所属的所有群组信息
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            idVec.push_back(atoi(row[0]));
        }
        mysql_free_result(res);
    }
    return idVec;
}