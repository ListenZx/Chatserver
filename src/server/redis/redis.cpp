#include"redis.hpp"
#include<iostream>
using namespace std;

Redis::Redis():_publish_conntext(nullptr),_subscribe_conntext(nullptr)
{
}
Redis::~Redis()
{
    if(_publish_conntext!=nullptr)
    {
        redisFree(_publish_conntext);
    }
    if(_subscribe_conntext!=nullptr)
    {
        redisFree(_subscribe_conntext);
    }
}

//连接redis服务器
bool Redis::connect()
{
    //负责public发布消息的上下文连接
    _publish_conntext=redisConnect("127.0.0.1",6379);
    if(_publish_conntext==nullptr)
    {
        cerr<<"connect redis failed!"<<endl;
        return false;
    }

    //负责subscribe发布消息的上下文连接
    _subscribe_conntext=redisConnect("127.0.0.1",6379);
    if( _subscribe_conntext==nullptr)
    {
        cerr<<"connect redis failed!"<<endl;
        return false; 
    }

    //在单独的线程中，监听通道上的事件，有消息给业务层进行上报——观察者
    //因为subsribe是线程阻塞的，需要通道对应单独的子线程来监听
    thread t([&](){
        obser_channel_message();
    });
    t.detach();
    return true;
}

//向redis指定的通道channel发布消息
bool Redis::publish(int channel,string messages)
{
    redisReply *reply=(redisReply *)redisCommand(_publish_conntext,"PUBLISH %d %s",channel,messages.c_str());
    if(reply==nullptr)
    {
        cerr<<"publish command failed!"<<endl;
        return false;
    }
    freeReplyObject(reply); //产生的都是动态资源 需要手动释放
    return true;
}

//向redis指定的通道subsribe订阅消息
/*为什么不使用上面的redisCommand：https://blog.csdn.net/QIANGWEIYUAN/article/details/97895611
    redisCommond:
        （1）redisAppendCommand：将命令缓存到本地
        （2）redisBufferWrite：将命令发送到redisServer上
        （3）redisGetReply：subsribe会以阻塞方式一直等到命令执行结果；PUBLISH马上响应
    SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息
    这里只做订阅通道，不接收通道消息
    通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    所以对于subsribe的redisCommond:不需要（3），需要手动完成该功能
*/
bool Redis::subscribe(int channel)
{
    //redisAppendCommand向 Redis 连接上下文添加命令
    if(REDIS_ERR==redisAppendCommand(this->_subscribe_conntext,"SUBSCRIBE %d",channel))
    {
        cerr<<"subscribe command failed!"<<"channel:"<<channel<<endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done=0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->_subscribe_conntext,&done))
        {
            cerr<<"subscribe command failed!"<<"channel:"<<channel<<endl;
            return false;
        }
    }
    //redisGetRepaky
    return true;
}

//向redis指定的通道unsubcribe取消订阅消息
bool Redis::unsunscribe(int channel)
{
    //redisAppendCommand向 Redis 连接上下文添加命令
    if(REDIS_ERR==redisAppendCommand(this->_subscribe_conntext,"UNSUBSCRIBE %d",channel))
    {
        cerr<<"unsubscribe command failed!"<<endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done=0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->_subscribe_conntext,&done))
        {
            cerr<<"unsubscribe command failed!"<<endl;
            return false;
        }
    }
    //redisGetRepaky
    return true;
}

//在独立线程中持续监听订阅通道上的消息，阻塞等待接收消息
void Redis::obser_channel_message()
{
    redisReply* reply=nullptr;
    while(REDIS_OK==redisGetReply(this->_subscribe_conntext,(void **)&reply))
    {
        //订阅收到的消息是一个带三元素的数组
        if(reply!=nullptr&&reply->element[2]!=nullptr&&reply->element[2]->str!=nullptr)
        {
            //给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
}

//初始化向业务层上报通道消息的回调接口
void Redis::init_notift_handler(function<void(int,string)> fn)
{
    this->_notify_message_handler=fn;
}

