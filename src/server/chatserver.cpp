#include "chatserver.hpp"
#include "json.hpp"
#include <functional>
#include <iostream>
#include"chatservice.hpp"

using namespace std;
using namespace placeholders;
using json=nlohmann::json;

//初始化聊天服务器对象
chatserver::chatserver(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
  _server.setConnectionCallback(std::bind(&chatserver::onConnetcion, this, _1));
  _server.setMessageCallback(std::bind(&chatserver::onMessage, this, _1, _2, _3));
  _server.setThreadNum(4);
}

//
void chatserver::start()
{
  _server.start();
}

//上报链接信息的相关回调函数
void chatserver::onConnetcion(const TcpConnectionPtr &conn)
{
  if (!conn->connected())
  {
    chatservice::instance()->clientCloseException(conn);
    conn->shutdown();
  }
}

//上报读写事件信息的回调函数
void chatserver::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
   string buf = buffer->retrieveAllAsString();
   // 测试，添加json打印代码
    cout << buf << endl;
   json js=json::parse(buf);//数据的反序列化
   
    // 达到的目的：完全解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"] 获取=》业务handler=》conn  js  time
  auto msgHandler=chatservice::instance()->getHandler(js["msgid"].get<int>());
  msgHandler(conn,js,time);
}

  