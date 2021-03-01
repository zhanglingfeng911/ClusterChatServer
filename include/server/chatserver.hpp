#ifndef CHATSERVER_H
#define CHATSERVER_H

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
// using namespace std;
using namespace muduo;
using namespace muduo::net;

class chatserver
{
private:
    TcpServer _server;
    EventLoop *_loop;
     //上报链接信息的相关回调函数
    void onConnetcion(const TcpConnectionPtr&);

    //上报读写事件信息的回调函数
    void onMessage(const TcpConnectionPtr&,
                            Buffer*,
                            Timestamp);


public:
    //初始化聊天服务器对象
    chatserver(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);

    //
    void start();
   
    
};


#endif