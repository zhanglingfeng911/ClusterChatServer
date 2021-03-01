#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <muduo/net/TcpConnection.h>
#include <functional>
#include "json.hpp"
#include "usermodel.hpp"
#include <mutex>
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

//处理消息的事件回调方法类型
using MsgHandler = function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

//聊天服务器服务类
class chatservice
{
private:
    unordered_map<int, MsgHandler> _msgHandlerMap;

    //定义互斥锁 保证usrConnMap的安全性
    mutex _connMutex;

    //存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _usrConnMap;
    chatservice(/* args */);
    //数据操作类对象
    Usermodel _userModel;
    offlineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    //redis操作对象
    Redis _redis;

public:
    static chatservice *instance();
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //1对1聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //服务器异常 业务重置方法
    void reset();

    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //加入群组业务
     void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    
    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

       // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void handleRedisSubscribeMessage(int userid, string msg);
};

#endif