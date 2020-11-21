//实现数据库操作，类似java的DAO层

#ifndef USERMODEL_H
#define USERMODEL_H

#include"user.hpp"

//User表的数据操作类
class UserModel{
public:
    //User表的插入
    bool insert(User& user);
    //User表根据id查询
    User query(int id);
    //更新用户的状态信息
    bool updateState(User user);
    //重置用户的状态信息
    void resetState();
};

#endif //USERMODEL_H