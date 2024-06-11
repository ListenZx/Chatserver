#include "chatserver.hpp"
#include "chatservice.hpp"
#include<iostream>
#include<signal.h>  //信号
using namespace std;


void resHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc,char **argv)
{
    signal(SIGINT,resHandler); //信号与槽
    EventLoop loop;
    //解析通过命令行参数传递的ip和port
    char* ip=argv[1];
    uint16_t port=atoi(argv[2]);
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"chatserver");
    server.start();
    loop.loop();
    return 0;
}