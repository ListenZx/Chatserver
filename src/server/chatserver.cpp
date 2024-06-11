#include "chatserver.hpp"
#include"chatservice.hpp"
#include<muduo/base/Logging.h>
#include<functional>
#include"json.hpp"
using namespace std;
using namespace placeholders;
using json=nlohmann::json; //json命名空间

//初始化服务器对象
ChatServer::ChatServer(EventLoop* loop,const InetAddress& listenAddr,const string& nameArg)
                        :_server(loop,listenAddr,nameArg),_loop(loop)    
{
    //注意
    //注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));

    //注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

    //设置线程个数
    _server.setThreadNum(4);
}

//启动服务
void ChatServer::start(){
    _server.start();
}

//上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    //客户端断开连接
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();//释放fd资源
    }
}

//上报读写信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr& conn,Buffer* buffer,Timestamp time)
{
    string buf=buffer->retrieveAllAsString(); 
    //数据的反序列化 该数据里面有msgid 识别消息类型进行处理
    json js=json::parse(buf);
    
    //达到目的：完全解耦网络模块的代码和业务模块的代码
    //希望通过js["msgid"]=》业务handler=》con js time
    
    auto msghandler=ChatService::instance()->getHandler(js["msgid"].get<int>()); //模板函数get<int>()进行类型转换成int
    //调用消息绑定好的事件处理器，来执行对应的业务处理
    msghandler(conn,js,time);
}

