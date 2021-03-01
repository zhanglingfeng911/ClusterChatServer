#include "chatserver.hpp"
#include <chatservice.hpp>
#include <iostream>
#include <signal.h>
using namespace std;

//处理服务器ctrl+c 结束后 重置User的状态信息
void restHandler(int)
{
    chatservice::instance()->reset();
    exit(0);
}

// int main()
// {
//     signal(SIGINT, restHandler);
//     EventLoop loop;
//     InetAddress addr("127.0.0.1", 6000);
//     chatserver server(&loop, addr, "ChatServer");
//     server.start();
//     loop.loop();

//     return 0;
// }
int main(int agrc, char **argv)
{

    if (agrc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }
    char *ip=argv[1];
    uint16_t port=atoi(argv[2]);
    signal(SIGINT, restHandler);
    EventLoop loop;
    InetAddress addr(ip, port);
    chatserver server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();

    return 0;
}