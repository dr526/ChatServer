#include <vector>
#include <muduo/base/Logging.h>
#include "chatservice.hpp"
#include "public.hpp"
using namespace std;
using namespace muduo;

//注册消息以及对应的回调操作
ChatService::ChatService()
{
    //用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});

    //群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)});

    //连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

//获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

//处理登陆业务 ORM:对象关系映射（分离业务层和数据层）
//{"msgid":1,"id":1,"password":"123456"}
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "login begin...";
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPassword() == pwd)
    {
        //不允许重复登陆
        if (user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登录";
            conn->send(response.dump());
        }
        else
        {
            //登陆成功，记录用户的连接信息
            //对共享变量的写操作，需要考虑线程安全
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            //id用户登陆成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            //登陆成功，更新用户状态信息
            //对数据库的操作，由mysql本身处理多线程的问题
            user.setState("online");
            _userModel.updateState(user);
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            //查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                //读取该用户的离线消息后，把该用户的所有离线消息删除
                _offlineMsgModel.remove(id);
            }

            //查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec_;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec_.push_back(js.dump());
                }
                response["friends"] = vec_;
            }

            //查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        //该用户不存在，登陆失败
        //该用户存在但是密码错误，登陆失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或者密码错误";
        conn->send(response.dump());
    }
}

//处理注册业务 name password
//{"msgid":3,"name":"dengrui","password":"123456"}
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    LOG_INFO << "reg begin....";
    string name = js["name"];
    string password = js["password"];
    User user;
    user.setName(name);
    user.setPassword(password);
    bool state = _userModel.insert(user);
    //注册成功
    if (state)
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    //注册失败
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errmessage"] = "reg failed!!!";
        conn->send(response.dump());
    }
    LOG_INFO << "reg end....";
}

//一对一聊天业务
//{"id":1,"msg":"hello dengrui","msgid":5,"name":"liguojun","to":2}
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    //获取对方id，用于发送消息
    int toId = js["toid"].get<int>();
    bool userState = false;
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toId);
        if (it != _userConnMap.end())
        {
            //对方在线，服务器转发消息,表明两者在同一台服务器登录
            it->second->send(js.dump());
            return;
        }
    }

    //查询toId是否在线，因为对方可能在另一台服务器上登陆
    User user = _userModel.query(toId);
    //对方在另一台服务器上登录
    if (user.getState() == "online")
    {
        _redis.publish(toId, js.dump());
        return;
    }

    //对方不在线，存储离线消息
    _offlineMsgModel.insert(toId, js.dump());
}

//添加好友业务 msgid id frendid
//{"msgid":6,"id":1,"friendid":2}
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    //当前用户的id
    int userId = js["id"].get<int>();
    //待添加好友的id
    int friendId = js["friendid"].get<int>();

    //存储好友信息
    _friendModel.insert(userId, friendId);
}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        //返回一个默认的处理器，空操作
        //=按值获取
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgid: " << msgid << " can not find handler";
        };
    }
    else
        return _msgHandlerMap[msgid];
}

//创建群组业务
//{"msgid":7,"groupname":"test","groupdesc":"just for fun"}
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    //存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        //存储群组创建人信息
        _groupModel.addGroup(userId, group.getId(), "creator");
    }
}

//加入群组业务
//{"msgid":8,"id":1,"groupid":1}
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js["id"].get<int>();
    int groupId = js["groupid"].get<int>();
    _groupModel.addGroup(userId, groupId, "normal");
}

//群组聊天业务
//{"msgid":9,"id":1,"groupid":1}
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js["id"].get<int>();
    int groupId = js["groupid"].get<int>();
    vector<int> userIdVec = _groupModel.queryGroupUsers(userId, groupId);
    lock_guard<mutex> lock(_connMutex);
    for (int &id : userIdVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            //转发群消息
            it->second->send(js.dump());
        }
        else
        {
            //查询toId是否在线
            User user = _userModel.query(id);
            //用户在其他服务器上登录
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            //存储离线群消息
            _offlineMsgModel.insert(id, js.dump());
        }
    }
}

//处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userId);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    //用户注销，相当于下线，在redis中取消订阅通道
    _redis.unsubscribe(userId);

    //更新用户的状态信息
    User user(userId, "", "", "offline");
    _userModel.updateState(user);
}

//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto i = _userConnMap.begin(); i != _userConnMap.end(); ++i)
        {
            if (i->second == conn)
            {
                //从map表删除用户的连接信息
                user.setId(i->first);
                _userConnMap.erase(i);
                break;
            }
        }
    }

    //用户异常退出，也相当于下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    //更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

//服务器异常，业务重置方法
void ChatService::reset()
{
    //把online状态的用户设置为offline
    _userModel.resetState();
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}