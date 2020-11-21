//映射类，相当于java中的实体类
#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

//User表的ORM类
class User
{
private:
    int id;
    string name;
    string password;
    string state;

public:
    User(int id = -1, string name = "", string password = "", string state = "offline")
    {
        this->id = id;
        this->name = name;
        this->password = password;
        this->state = state;
    }
    void setId(int id)
    {
        this->id = id;
    }
    void setName(string name)
    {
        this->name = name;
    }
    void setPassword(string passsword)
    {
        this->password = passsword;
    }
    void setState(string state)
    {
        this->state = state;
    }

    int getId()
    {
        return this->id;
    }
    string getName()
    {
        return this->name;
    }
    string getPassword()
    {
        return this->password;
    }
    string getState()
    {
        return this->state;
    }
};

#endif //USER_H