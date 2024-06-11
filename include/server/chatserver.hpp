#ifndef CHATSERVER_H
#define CHATSERVER_H

#include<iostream>
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<string>
using namespace std;
using namespace muduo;
using namespace muduo::net;

class ChatServer{
public:
        //初始化服务器对象
        ChatServer(EventLoop* loop,
                        const InetAddress& listenAddr,
                        const string& nameArg);
        //启动服务器
        void start();
private:
        //上报链接相关信息的回调函数
        void onConnection(const TcpConnectionPtr& conn);
        //上报读写信息的回调函数
        void onMessage(const TcpConnectionPtr& conn,
                        Buffer* buf,
                        Timestamp time);

        TcpServer _server;  //组合的muduo库，实现服务器功能的类对象
        EventLoop* _loop;   //指向事件循环的指针

};
#endif