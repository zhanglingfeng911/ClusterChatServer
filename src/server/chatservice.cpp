#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
#include <iostream>

using namespace std;
chatservice *chatservice::instance()
{
    static chatservice service;
    return &service;
}

//注册消息以及对应的回调操作
chatservice::chatservice(/* args */)
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&chatservice::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&chatservice::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&chatservice::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEN_MSG, std::bind(&chatservice::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&chatservice::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&chatservice::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&chatservice::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&chatservice::loginout, this, _1, _2, _3)});

    if (_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&chatservice::handleRedisSubscribeMessage, this, _1, _2));
    }
}

MsgHandler chatservice::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << " msgid" << msgid << "can not find handler";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

//处理登录业务 id  pwd pwd
void chatservice::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online") //用户已经登陆 不允许重复登陆
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using ,input another!";
            conn->send(response.dump());
        }
        else //登陆成功 更新用户状态信息 state offline->online
        {
            //减小锁的粒度 增加并发量
            {
                lock_guard<mutex> lock(_connMutex);
                //登陆成功 记录用户连接信息
                _usrConnMap.insert({id, conn});
            }

            //id用户登陆成功后 向redis订阅channel(id)
            _redis.subscribe(id);

            //登陆成功 更新用户状态信息 state:offline->online
            user.setState("online");
            //test:
            cout << user.getState() << endl;

            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            //查询是否有离线消息 包括onechat和groupchat存储的离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                //读取用户的离线消息后 把它删除掉
                _offlineMsgModel.remove(id);
            }

            //查询用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();

                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2; //{"id:***,name:***,state:***","id:***,name:***,state:***","id:***,name:***,state:***","id:***,name:***,state:***",}
            }
            //查询用户的群组消息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                vector<string> groupVec;
                for (Group &group : groupuserVec)
                {
                    json groupjs; //{"id:**","groupname:**","groupdesc:***""suers:***"",","id:**","groupname:**","groupdesc:***""suers:***","id:**","groupname:**","groupdesc:***",}
                    groupjs["id"] = group.getId();
                    groupjs["groupname"] = group.getName();
                    groupjs["groupdesc"] = group.getDesc();

                    vector<string> userVec;
                    for (GroupUser &groupuser : group.getUsers())
                    {
                        json userjs;
                        userjs["id"] = groupuser.getId();
                        userjs["name"] = groupuser.getName();
                        userjs["state"] = groupuser.getState();
                        userjs["role"] = groupuser.getRole();

                        userVec.push_back(userjs.dump());
                    }
                    groupjs["users"] = userVec;
                    groupVec.push_back(groupjs.dump());
                }

                response["groups"] = groupVec;
            }

            conn->send(response.dump());
        }
    }
    else //用户不存在 或者用户存在但是密码错误
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}

//处理注册服务
void chatservice::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);

    bool state = _userModel.insert(user);
    if (state)
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理注销业务
void chatservice::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {

        lock_guard<mutex> lock(_connMutex);
        auto it = _usrConnMap.find(userid);
        if (it != _usrConnMap.end())
        {
            _usrConnMap.erase(it);
        }
    }

    //用户注销 就是下线 取消订阅通道
    _redis.unsubscribe(userid);
    //更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

void chatservice::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;

    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _usrConnMap.begin(); it != _usrConnMap.end(); ++it)
        {
            if (it->second = conn)
            {
                user.setId(it->first);
                _usrConnMap.erase(it);
                break;
            }
        }
    }

    _redis.unsubscribe(user.getId());
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}
void chatservice::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{

    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _usrConnMap.find(toid);
        if (it != _usrConnMap.end())
        {
            //toid 在线 转发消息
            it->second->send(js.dump()); //it->second是一条客户端与服务器的连接  服务器通过此连接可以给客户端发送消息
            return;
        }
    }
    //查询toid是否在线 因为可能toid不在本服务器上在线 但在其他服务器上 可以通过查询数据库查询其是否在线
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }
    else
    {
        //toid 不在线 存储离线消息
        _offlineMsgModel.insert(toid, js.dump());
    }
}

void chatservice::reset()
{
    //把online状态的用户设置成Offline
    _userModel.reserState();
}

//添加好有业务 msg id friendid
void chatservice::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{

    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //存储好友信息
    _friendModel.insert(userid, friendid);
}

//创建群组业务
void chatservice::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    //存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        //存储创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

//加入群组业务
void chatservice::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

//群组聊天业务 离线群消息是发给用户的
void chatservice::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{

    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid); //群众其他用户的userid
    lock_guard<mutex> lock(_connMutex);
    for (int &id : useridVec)
    {
        auto it = _usrConnMap.find(id);
        if (it != _usrConnMap.end())
        {
            it->second->send(js.dump()); //如果用户在线 将消息发给群组中其他userid
        }
        else
        {
            // 查询toid是否在线 和onechat一样的道理
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }

            // _offlineMsgModel.insert(id, js.dump()); //如果用户不在线 将消息存到offlinemessage表中
        }
    }
}

// 从redis消息队列中获取订阅的消息
void chatservice::handleRedisSubscribeMessage(int userid, string msg)
{
    // json js=json::parse(msg.c_str());
    lock_guard<mutex> lock(_connMutex);
    auto it = _usrConnMap.find(userid);
    if (it != _usrConnMap.end())
    {
        it->second->send(msg);
        // it->second->send(js.dump());
        return;
    }
    else
    {
        // 存储该用户的离线消息
        _offlineMsgModel.insert(userid, msg);
    }
}