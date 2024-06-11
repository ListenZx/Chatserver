#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include<muduo/net/TcpConnection.h>
#include<unordered_map>  //消息id映射
#include<functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;
#include <mutex>
#include "json.hpp"
using json=nlohmann::json; 

#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include "redis.hpp"
//MsgHandler 是一个类型别名——表示处理消息的事件回调的类型
using MsgHandler=function<void(const TcpConnectionPtr&,json&,Timestamp)>;

//聊天服务器业务类
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //获取消息对应类型的处理器
    MsgHandler getHandler(int msgid);
    //客户端异常退出处理业务：将当前登录的用户进行退出
    void clientCloseException(const TcpConnectionPtr& conn);
    //服务器异常，业务重置方法
    void reset();
    //登录处理业务
    void login(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //注册登处理业务
    void reg(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //点对点通信业务
    void oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //加入群组业务
    void addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //用户注销业务
    void loginout(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //从redis消息队列中获取订阅的消息
    void handlerRedisSubscribeMessage(int userid,string msg);
private:
    ChatService();
    ChatService(const ChatService& other)=delete;
    ChatService& operator=(ChatService& other)const=delete;

    //存储消息id和其对应业务的处理方法
    unordered_map<int,MsgHandler> _msgHandlerMap;
    //存储在线用户的通信连接
    unordered_map<int,TcpConnectionPtr> _userConnMap;
    //定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;
    
    //数据操作类对象
    UserModel _userModel;    //用户操作对象
    OfflineMessageModel _offlineMessageModel;    //离线消息表操作对象
    FriendModel _friendModel;  //好友关系表操作对象
    GroupModel _groupModel; //群组操作对象

    //redis操作对象
    Redis _redis;
};



#endif