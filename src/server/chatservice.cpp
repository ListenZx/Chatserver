#include "chatservice.hpp"
#include "public.hpp"
#include "user.hpp"


#include<string>
using namespace std;
#include<muduo/base/Logging.h> //日志库

//获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

//注册消息以及对应的Handler
ChatService:: ChatService() 
{
    //用户业务管理相关事件处理回调注册
    _msgHandlerMap.insert({MSG_TYPE_LOGIN_REQUIRE,bind(&ChatService::login,this,_1,_2,_3)}); //登陆
    _msgHandlerMap.insert({MSG_TYPE_REG_REQUIRE,bind(&ChatService::reg,this,_1,_2,_3)}); //注册
    _msgHandlerMap.insert({MSG_TYPE_ONE_CHAT,bind(&ChatService::oneChat,this,_1,_2,_3)});   //点对点聊天
    _msgHandlerMap.insert({MSG_TYPE_ADDFRIEND_REQUIRE,bind(&ChatService::addFriend,this,_1,_2,_3)}); //添加好友
    _msgHandlerMap.insert({MSG_TYPE_CREATEGROUP_REQUIRE,bind(&ChatService::createGroup,this,_1,_2,_3)}); //创建群组

    //群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({MSG_TYPE_ADDGROUP_REQUIRE,bind(&ChatService::addGroup,this,_1,_2,_3)}); //添加好友
    _msgHandlerMap.insert({MSG_TYPE_GROUP_CHAT,bind(&ChatService::groupChat,this,_1,_2,_3)}); //群发消息
    _msgHandlerMap.insert({MSG_TYPE_LOGINOUT_REQUIRE,bind(&ChatService::loginout,this,_1,_2,_3)});//用户注销;

    //连接redis服务器
    if(_redis.connect())
    {
        //设置上报消息的回调
        _redis.init_notift_handler(bind(&ChatService::handlerRedisSubscribeMessage,this,_1,_2));
    }

}

//对应的Handler的回调操作
MsgHandler ChatService::getHandler(int msgid)
{
    //记录错误日志 ，msgid没有对应的事件处理回调
    auto it=_msgHandlerMap.find(msgid); //不要用[]进行查询，如果没有会自动添加
    if(it==_msgHandlerMap.end())
    {
       //返回一个默认的处理器，空处理器，该处理器会打印错误日志
        return [=](const TcpConnectionPtr &conn,json &js,Timestamp time)->void
        {
              LOG_ERROR<<"msgid："<<msgid<<"can not find handler!"; 
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

//登录处理业务
void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    LOG_INFO<<"do login service!!";
    int id=js["id"];
    string pwd=js["password"];
    User user=_userModel.queryid(id);
    if(user.getId()==id&&user.getPwd()==pwd)
    {
        if(user.getState()=="online")
        {
            //该用户已存在，不允许重复登录
            json response;
            response["msgid"]=MSG_TYPE_LOGIN_RESPOND;
            response["error"]=1;
            response["errmsg"]="The account has been logged in, please enter the account again!";
            conn->send(response.dump());
        }
        else
        {
            //登录成功
            
            //保存连接到在线用户表
            {//作用域控制：线程互斥范围{} ，锁的力度小：体现多线程互斥锁的作用，力度太大就是串行了
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id,conn});
            }

            //id用户登陆成功，向redis订阅channel(id)
            _redis.subscribe(id);

            //更新用户的状态信息offline=》online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"]=MSG_TYPE_LOGIN_RESPOND;
            response["error"]=0;
            response["id"]=user.getId();
            response["name"]=user.getName();

            //查询该用户是否有离线消息
            vector<string>vec=_offlineMessageModel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"]=vec;
                //读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMessageModel.remove(id);
            }
            
            //查询好友信息并返回
            vector<User>friendVec=_friendModel.query(id);
            if(!friendVec.empty())
            {
                //无法直接用json存储
                //response["friends"]=friendVec; //friendVec的自定义类型元素无法序列化
                //通过json将每个好友的信息装换成字符串=》存储在字符串vector里面=》在用response["friends"]序列化存储
                vector<string>temp;
                for(auto& it:friendVec)
                {
                    json js;
                    js["id"]=it.getId();
                    js["name"]=it.getName();
                    js["state"]=it.getState();
                    temp.push_back(js.dump());
                }
                response["friend"]=temp;
            }

            //查询群组信息并返回
            vector<Group>groupVec=_groupModel.queryGroups(id);
            if(!groupVec.empty())
            {
                vector<string>temp;
                for(Group &group:groupVec)
                {
                    json js1;
                    js1["id"]=group.getId();
                    js1["name"]=group.getName();
                    js1["desc"]=group.getDesc();     
                    vector<string>userVec;
                    for(GroupUser &user:group.getUsersVec())
                    {
                        json js;
                        js["id"]=user.getId();
                        js["name"]=user.getName();
                        js["state"]=user.getState();
                        js["role"]=user.getRole();
                        userVec.push_back(js.dump());
                    }
                    js1["users"]=userVec;
                    temp.push_back(js1.dump());
                }
                response["groups"]=temp;
            }
            conn->send(response.dump());
        }
    }
    else 
    {
        //该用户不存在，用户存在但是密码错误，登陆失败
        json response;
        response["msgid"]=MSG_TYPE_LOGIN_RESPOND; 
        response["error"]=2;
        response["errmsg"]="The user does not exist or the password is wrong!";
        conn->send(response.dump());
    }
}
//{"msgid":1,"id":1,"password":"ljlj"}
  
//注册登处理业务:注册消息 name passward
void ChatService::reg(const TcpConnectionPtr &conn,json &js,Timestamp)
{
    string name=js["name"];
    string pwd=js["password"];

    LOG_INFO<<"do reg require__name:"<<name<<"__password:"<<pwd;

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool flag=_userModel.insert(user);
    if(flag)
    {
        //注册成功
        json response;
        response["msgid"]=MSG_TYPE_REG_RESPOND;
        response["error"]=0;
        response["id"]=user.getId();
        conn->send(response.dump());
    }
    else
    {
        //注册失败
        json response;
        response["msgid"]=MSG_TYPE_REG_RESPOND;
        response["error"]=1;
        conn->send(response.dump());
    }
}
//{"msgid":3,"name":"zx","password":"ljlj"}/

//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr& conn)
{
    User user;
    {   //多线程互斥锁保护_userConnMap线程安全
        lock_guard<mutex> lock(_connMutex); 
        for(auto it=_userConnMap.begin();it!=_userConnMap.end();++it)
        {
            if(it->second==conn)
            {
                //从map表删除用户的连接消息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    //更新用户的状态信息
    if(user.getId()!=-1)
    {
        //用户异常退出，相当于下线，在redis取消订阅通道
        _redis.unsunscribe(user.getId());
        user.setState("offline");
        _userModel.updateState(user);
    }
}

//用户注销业务
void ChatService::loginout(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["id"].get<int>();
    {
        lock_guard<mutex>lock(_connMutex);
        for(auto it=_userConnMap.begin();it!=_userConnMap.end();++it)
        {
            if(it->first==userid)
            {
                _userConnMap.erase(it);
            }
        }
    }
    User user(userid,"","","offline");
    _userModel.updateState(user);
    //用户注销，相当于下线，在redis取消订阅通道
    _redis.unsunscribe(user.getId());
}

//一对一通信业务
void ChatService::oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int toid=js["toid"].get<int>();
    {
        lock_guard<mutex>lock(_connMutex);
        auto it=_userConnMap.find(toid);
    //1，在线且同一个服务器
        if(it!=_userConnMap.end())
        {
            //toid 在线，转发消息 服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    //2，在线且不在同一个服务器
    User user=_userModel.queryid(toid);
    if(user.getState()=="online")
    {
        _redis.publish(toid,js.dump());
        return;
    }

    //3，不在线
    //toid不在线，存储离线消息
    _offlineMessageModel.insert(toid,js.dump());
}
//{"msgid":5,"fromname":"zx","fromid":1,"toid":3,"msg":"hello?"}

//服务器异常，业务重置方法
void ChatService::reset()
{
    //把online状态的用户，设置成offline
    _userModel.resetState();
}

//添加好友业务 
void ChatService::addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int id=js["id"].get<int>();
    int friendid=js["friendid"].get<int>();
    if(_userModel.queryid(friendid).getId()==-1) //判断好友id是否存在
    {
        //不存在好友id
        json response;
        response["error"]=1;
        response["errmsg"]="The added account does not exist!";
        conn->send(response.dump());
    }
    else
    {  
         //存在id
        _friendModel.insert(id,friendid); //存储好友信息
        json response;
        response["error"]=0;
        conn->send(response.dump());
    }
//int id=js["id"].get<int>()  &&  stoi(js["id"].dump())
//js["id"].get<int>() 是直接从特定的数据结构中按照整数类型获取对应值，这个过程可能涉及到该数据结构内部特定的转换和提取逻辑。
//stoi(js["id"].dump()) 则是先将数据结构中的值转换为字符串表示（通过 dump 方法），然后再将这个字符串用 stoi 函数转换为整数
//后者多了一个中间的字符串转换步骤。
//在某些情况下，可能会产生不同的行为或结果，比如如果 dump 过程产生的字符串不符合 stoi 的要求，可能会导致错误，而直接的 get<int> 可能就不会有这样的问题。但具体还需要结合实际使用的场景和数据结构的具体特性来分析。
}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["userid"].get<int>();
    string name=js["groupname"];
    string desc=js["groupdesc"];
    //存储新创建的群组信息
    Group group(-1,name,desc);
    if(_groupModel.createGroup(group))
    {
        //存储群组创建人信息
        _groupModel.addGroup(userid,group.getId(),"creator");
    }
}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["userid"].get<int>();
    int groupid=js["groupid"].get<int>();
    _groupModel.addGroup(userid,groupid,"normal");
}

//群发消息业务
void ChatService::groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["userid"].get<int>();
    int groupid=js["groupid"].get<int>();
    vector<int> useridVec=_groupModel.queryGroupUsers(userid,groupid);
    {
        lock_guard<mutex> lock(_connMutex); //对_userConnMap操作加锁
        for(int& id:useridVec)
        {
            auto it=_userConnMap.find(id);
        //1，在线且同一个服务器
            if(it!=_userConnMap.end())
            {
                //转发群消息
                it->second->send(js.dump());
            }
            else
            {
        //2，在线且不在同一个服务器
            User user=_userModel.queryid(id);
            if(user.getState()=="online")
            {
                _redis.publish(id,js.dump());
                return;
            }

        //3,不在线
                //存储离线消息
                _offlineMessageModel.insert(id,js.dump());
            }
        }
    }
}

//从redis消息队列中获取订阅的消息
void ChatService::handlerRedisSubscribeMessage(int userid,string msg)
{
    //防止获取消息的时候用户下线
    lock_guard<mutex>lock(_connMutex);
    auto it=_userConnMap.find(userid);
    if(it!=_userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    //存储该用户的离线消息
    _offlineMessageModel.insert(userid,msg);
}