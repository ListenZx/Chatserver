#ifndef REDIS_H
#define REDIS_H

#include<hiredis/hiredis.h>
#include<thread>
#include<functional>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    //连接redis服务器
    bool connect();

    //向redis指定的通道channel发布消息
    bool publish(int channel,string messages);

    //向redis指定的通道subsribe订阅消息
    bool subscribe(int channel);

    //向redis指定的通道unsubscribe取消订阅消息，用户下线消息就发到离线消息表
    bool unsunscribe(int channel);

    //在独立线程中接收订阅通道中的消息
    void obser_channel_message();
    
    //初始化向业务层上报通道消息的回调函数
    void init_notift_handler(function<void(int,string)> fn);
private:

//这里定义两个上下文对象 因为subscribe订阅消息就会阻塞
    //hiredis同步上下文对象，负责publish消息
    redisContext *_publish_conntext;

    //hiredis同步上下文对象，负责subscribe订阅消息
    redisContext *_subscribe_conntext;

    //回调操作，接收订阅的消息，给service层上报，参数：redis消息类型 "message",int,string取第二个通道和消息内容
    function<void(int,string)> _notify_message_handler;
};

#endif