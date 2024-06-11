#include"json.hpp"
#include<iostream>
#include<thread>
#include<string>
#include<vector>
#include<chrono>
#include<ctime>
using namespace std;
using json=nlohmann::json;

#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include"group.hpp"
#include"user.hpp"
#include"public.hpp"

//记录当前系统登陆的用户信息
User g_currrendUser;
//记录当前登陆用户的好友列表信息
vector<User> g_currentUserFriendList;
//记录当前登陆用户的群组列表信息
vector<Group> g_currentUserGroupList;
//表示用户有没有注销 注销的适合表示不在循环主聊天页面
bool isMainMenuRuning=false;

//显示当前登陆成功用户的基本信息
void showCurrentUserData();
//接收线程
void readTaskHandler(int clientfd);
//获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
//主聊天页面程序
void mainMenu(int clientfd);

//主页面支持的客户端命令处理函数        //在定义处可以不重复写默认参数，但这并不影响这些默认参数仍然是有效的。
void help(int=0,string=""); //必须要（int，string）要和上面处理Map中function类型对应函数声明中已经指定了默认参数后
void chat(int,string); 
void addfriend(int,string);
void creategroup(int,string);
void addgroup(int,string);
void groupchat(int,string);
void loginout(int,string);

//主页面支持的客户端命令处理格式列表
unordered_map<string,string>commandMap=
{
    {"help","显示所有支持的命令，格式 help"},
    {"chat","一对一聊天，格式 chat:friendid:message"},
    {"addfriend","添加好友，格式 addfriend:friendid"},
    {"creategroup","创建群组，格式 creategroup:groupname:groupdesc"},
    {"addgroup","加入群组，格式 addgroup:groupid"},
    {"groupchat","群聊，格式 groupchat:groupid:message"},
    {"loginout","注销，格式  loginout"}
};


//主页面支持的客户端命令处理Map
unordered_map<string,function<void(int,string)>>commandHandlerMap=
{
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"loginout", loginout}
};



//聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc,char **argv)
{
    if(argc<3)
    {
        cerr<<"command invalid! example:./ChatClient 127.0.0.1 6000"<<endl;//准错误输出流（cerr)
    }
    
    //解析通过命令行参数传递ip和port
    char* ip=argv[1];
    uint16_t port=atoi(argv[2]);

    //创建client端的socket
    int clientfd=socket(AF_INET,SOCK_STREAM,0);
    if(-1==clientfd)
    {
        cerr<<"socket create error"<<endl;
        exit(-1);
    }
    
    //填写client需要的server信息的ip+port
    sockaddr_in server;
    memset(&server,0,sizeof(sockaddr));

    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr=inet_addr(ip);
    if(-1==connect(clientfd,(sockaddr *)&server,sizeof(sockaddr_in)))
    {
        cerr<<"connect server error"<<endl;
        close(clientfd);
        exit(-1);
    }
    else
    {
        cout<<"connect server suucess!"<<endl;
    }
    //main线程用于接收用户输入，负责发送数据
    for(;;)
    {
        //显示首页面菜单 登录、注册、退出
        cout<<"===================="<<endl;
        cout<<"1.login"<<endl;
        cout<<"2.register"<<endl;
        cout<<"3.quit"<<endl;
        cout<<"===================="<<endl;
        cout<<"choice:";
        int choice=0;
        cin>>choice;
        cin.get();//清cin>>掉缓冲区回车
        switch(choice)
        {
            case 1: //login业务
            {
                int id=0;
                char pwd[50]={0};
                cout<<"userid:";
                cin>>id;
                cin.get();
                cout<<"userpassword:";
                cin.getline(pwd,50);

                json js;
                js["msgid"]=MSG_TYPE_LOGIN_REQUIRE;
                js["id"]=id;
                js["password"]=pwd;
                string request=js.dump();
                int len=send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
                if(len==-1)
                {
                    cerr<<"send login msg error:"<<request<<endl;
                }
                else
                {
                    char buffer[1024]={0};
                    len=recv(clientfd,buffer,1024,0);
                    if(-1==len)
                    {
                        cerr<<"recv login response error"<<endl;
                    }
                    else
                    {
                        
                        json responsejs=json::parse(buffer);
                        if(0!=responsejs["error"].get<int>())
                        {
                            cerr<<responsejs["errmsg"]<<endl;
                        }
                        else//登陆成功
                        {
                            //清空之前用户的信息
                            g_currrendUser=User();
                            g_currentUserFriendList.clear();
                            g_currentUserGroupList.clear();


                            //记录当前用户的id和name
                            g_currrendUser.setId(responsejs["id"].get<int>());
                            g_currrendUser.setName(responsejs["name"]);
                            //记录当前用户的好友列表
                            if(responsejs.contains("friend"))//contain（key） 看json包不包含key
                            {
                                vector<string>vec=responsejs["friend"];
                                for(string &it:vec)
                                {
                                    json js=json::parse(it);
                                    User user;
                                    user.setId(js["id"]);
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }
                            //记录当前用户的群组列表信息
                            if(responsejs.contains("groups"))
                            {
                                vector<string>vec=responsejs["groups"];
                                for(string &groupstr:vec)
                                {
                                    json js=json::parse(groupstr);
                                    Group group;
                                    group.setId(js["id"]);
                                    group.setName(js["name"]);
                                    group.setDesc(js["desc"]);
                                    vector<string>vec2=js["users"];
                                    for(string &userstr:vec2)
                                    {
                                        json js1=json::parse(userstr);
                                        GroupUser user;
                                        user.setId(js1["id"].get<int>());
                                        user.setName(js1["name"]);
                                        user.setState(js1["state"]);
                                        user.setRole(js1["role"]);
                                        group.getUsersVec().push_back(user);
                                    }

                                    g_currentUserGroupList.push_back(group);
                                }
                            }
                            //显示用户的基本信息
                            showCurrentUserData();

                            //显示当前用户的离线消息 个人聊天消息和群组消息
                            if(responsejs.contains("offlinemsg"))
                            {
                                cout<<"Your offline messages: >>"<<endl;
                                json js=json::parse(buffer);
                                vector<string>vec=responsejs["offlinemsg"];
                                for(string &it:vec)
                                {
                                    json js=json::parse(it);
                                    int MSG_TYPE=js["msgid"].get<int>();
                                    //一对一的消息
                                    if(MSG_TYPE_ONE_CHAT==MSG_TYPE)
                                    {
                                    // cout<<js["time"].get<string>()<<" ["<<js["id"]<<"] "<<js["name"]<<" said："<<js["msg"]<<endl;
                                        cout<<"user["<<js["fromid"]<<"] "<<js["fromname"]<<" said："<<js["msg"]<<endl;
                                    }
                                    //群聊中的群发消息 
                                    if(MSG_TYPE_GROUP_CHAT==MSG_TYPE)
                                    {
                                        //  cout<<js["time"].get<string>()<<" ["<<js["groupid"]<<"] "<<js["userid"]<<" said："<<js["msg"]<<endl;
                                        cout<<"group["<<js["groupid"]<<"] "<<js["userid"]<<" said："<<js["msg"]<<endl;
                                    }
                                }
                            }

                            //登录成功 创建唯一的接收线程负责接收数据
                            static int readThreadNum;
                            if(readThreadNum==0)
                            {
                                thread readTask(readTaskHandler,clientfd);//pthread create
                                readTask.detach();  //pthread detach
                            }

                            //进入聊天主菜单页面
                            isMainMenuRuning=true;
                            mainMenu(clientfd);
                        }
                    }
                }
                break;
            }
            case 2: //register业务
            {
                char name[50]={0};
                char pwd[50]={0};
                cout<<"username:";
                cin.getline(name,50); //名字需要空格
                cout<<"userpassword:";
                cin.getline(pwd,50);

                json js;
                js["msgid"]=MSG_TYPE_REG_REQUIRE;
                js["name"]=name; 
                js["password"]=pwd;
                string request=js.dump();
                int len=send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
                if(len==-1)
                {
                    cerr<<"send reg msg error:"<<request<<endl;
                }
                else 
                {
                    char buffer[1024]={0};
                    len=recv(clientfd,buffer,1024,0);//recv阻塞等待
                    if(len==-1)
                    {
                        cerr<<"recv reg response error"<<endl;
                    }
                    else
                    {
                        json responsejs=json::parse(buffer);
                        if(responsejs["error"].get<int>()!=0)
                        {
                            cerr<<name<<"is already exist, register error!"<<endl;//具体注册错误后台优点检测 应该通过responsejs["errmsg"]返回
                        }
                        else//注册成功
                        {
                            cout<<"reg success!"<<"id:"<<responsejs["id"]<<" do not forget it!"<<endl;
                        }
                    }

                }
                break;
            }
            case 3: //quit业务
            {
                close(clientfd);
                exit(0);
            }
            default:
            {
                cout<<"invalid input!"<<endl;
                break;
            }
        }
    }
}


//主聊天页面程序
void mainMenu(int clientfd)
{
    help();
    char buffer[1024]={0};
    while(isMainMenuRuning)
    {
        cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command;//存储命令
        int index=commandbuf.find(":");
        if(index==-1)
        {
            command=commandbuf;
        }
        else
        {
            command=commandbuf.substr(0,index);
        }
        auto it=commandHandlerMap.find(command);
        if(it==commandHandlerMap.end())
        {
            cerr<<"invalid input command!"<<endl;
            continue;
        }
        //调用对应命令事件处理回调，满足开闭原则：mainMenu对修改封闭，添加新的功能不需要修改该函数
        it->second(clientfd,commandbuf.substr(index+1,commandbuf.size()-index));
    }
}

// {"help","显示所有支持的命令，格式 help"},
void help(int fd,string str)
{
    cout<<"show command list >>>"<<endl;
    for(auto &p:commandMap)
    {
        cout<<p.first<<":"<<p.second<<endl;
    }
    cout<<endl;
}
//     {"addfriend","添加好友，格式 addfriend:friendid"},
void addfriend(int clientfd,string str)
{
    int friendid=atoi(str.c_str());
    json js;
    js["msgid"]=MSG_TYPE_ADDFRIEND_REQUIRE;
    js["id"]=g_currrendUser.getId();
    js["friendid"]=friendid;
    string buffer=js.dump();
    int len= send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"send addfriend msg error -> "<<buffer<<endl;
    }
    else
    {
        char buffer[1024]={0};
        len=recv(clientfd,buffer,1024,0);//recv阻塞等待
        if(-1==len)
        {
            cerr<<"recv addfriend response error -> "<<buffer<<endl;
        }
        else
        {
            json responsejs=json::parse(buffer);
            if(responsejs["error"].get<int>()!=0)
            {
                cerr<<responsejs["errmsg"]<<endl;
            }
            else
            {
                cout<<"add friends success!"<<endl; //bug 重复添加待解决
            }
        }
    }
}

//     {"chat","一对一聊天，格式 chat:friendid:message"},
void chat(int clientfd,string str)
{
    int index=str.find(":");
    if(index==-1)
    {
        cerr<<"chat command invalid!"<<endl;
        return;
    }
    int id=atoi(str.substr(0,index).c_str());
    string message=str.substr(index+1,str.size()-index);
    json js;
    js["msgid"]=MSG_TYPE_ONE_CHAT;
    js["fromid"]=g_currrendUser.getId();
    js["fromname"]=g_currrendUser.getName();
    js["toid"]=id;
    js["msg"]=message;
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"send chat msg error -> "<<buffer<<endl;
    }
}

//     {"creategroup","创建群组，格式 creategroup:groupname:groupdesc"},
void creategroup(int clientfd,string str)
{
    int index=str.find(":");
    if(index==-1)
    {
        cerr<<"creategroup command invalid!"<<endl;
        return;
    }
    string groupname=str.substr(0,index);
    string groupdesc=str.substr(index+1,str.size()-index-1);
    json js;
    js["msgid"]=MSG_TYPE_CREATEGROUP_REQUIRE;
    js["userid"]=g_currrendUser.getId();
    js["groupname"]=groupname;
    js["groupdesc"]=groupdesc;
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"send creategroup msg error -> "<<buffer<<endl;
    }
    
}

//     {"addgroup","加入群组，格式 addgroup:groupid"},
void addgroup(int clientfd,string str)
{
    string groupid=str;
    json js;
    js["msgtype"]=MSG_TYPE_ADDGROUP_REQUIRE;
    js["groupid"]=groupid;
    js["userid"]=g_currrendUser.getId();
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"send addgroup msg error -> "<<buffer<<endl;
    }
}

//     {"groupchat","群聊，格式 groupchat:groupid:message"},
void groupchat(int clientfd,string str)
{
    int index=str.find(":");
    if(index==-1)
    {
       cerr<<"groupchat command invalid!"<<endl;
       return;
    }
    int groupid=atoi(str.substr(0,index).c_str());
    string message=str.substr(index+1,str.size()-index-1);
    json js;
    js["msgid"]=MSG_TYPE_GROUP_CHAT;
    js["userid"]=g_currrendUser.getId();
    js["name"]=g_currrendUser.getName();    
    js["groupid"]=groupid;
    js["msg"]=message;
    //js["time"]=getCurrentTime();

    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"send groupchat msg error -> "<<buffer<<endl;
    }
}

//     {"loginout","注销，格式  loginout"}
void loginout(int clientfd,string str)
{
    json js;
    js["msgid"]=MSG_TYPE_LOGINOUT_REQUIRE;
    js["id"]=g_currrendUser.getId();
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"send loginout msg error -> "<<buffer<<endl;

    }
    else
    {
        isMainMenuRuning=false;
        cout<<"bye bye!"<<endl;
    }
        
}

//显示当前登陆成功用户的基本信息
void showCurrentUserData()
{
    cout<<"=====================login user information======================="<<endl;
    cout<<"current login user => id:"<<g_currrendUser.getId()<<" name:"<<g_currrendUser.getName()<<endl;
    cout<<"---------------------friend list----------------------"<<endl;
    if(!g_currentUserFriendList.empty())//有好友
    {
        for(User& user:g_currentUserFriendList)
        {
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }
     cout<<"--------------------friend end-----------------------"<<endl;

    cout<<"---------------------group list----------------------"<<endl;
    if(!g_currentUserGroupList.empty())//有群组
    {
        for(Group& group:g_currentUserGroupList)
        {
            cout<<"***group***"<<" id:"<<group.getId()<<" name:"<<group.getName()<<" descL:"<<group.getDesc()<<endl;
            for(GroupUser& user:group.getUsersVec())
            {
                cout<<"user"<<" role:"<<user.getRole()<<" id:"<<user.getId()<<" name:"<<user.getName()<<" state:"<<user.getState()<<endl;
            }
        }
    }
    cout<<"--------------------group end-----------------------"<<endl;
    cout<<"=======================end==========================="<<endl;
}

//获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    return "2024/6/7";
}

//接收线程
void readTaskHandler(int clientfd)
{
    for(;;)
    {
        char buffer[1024]={0};
        int len=recv(clientfd,buffer,1024,0);
        if(-1==len||0==len)
        {
            close(clientfd);
            exit(-1);
        }

        //接收ChatServer转发的数据，反序列化生成json数据对象
        json js=json::parse(buffer);
        //不同消息的处理 其实可以封装起来！ 做成接口 不用if else
        int MSG_TYPE=js["msgid"].get<int>();
        //一对一的消息
        if(MSG_TYPE_ONE_CHAT==MSG_TYPE)
        {
           // cout<<js["time"].get<string>()<<" ["<<js["id"]<<"] "<<js["name"]<<" said："<<js["msg"]<<endl;
            cout<<"user["<<js["fromid"]<<"] "<<js["fromname"]<<" said："<<js["msg"]<<endl;
            continue;
        }
        //群聊中的群发消息 
        if(MSG_TYPE_GROUP_CHAT==MSG_TYPE)
        {
            //  cout<<js["time"].get<string>()<<" ["<<js["groupid"]<<"] "<<js["userid"]<<" said："<<js["msg"]<<endl;
            cout<<"group["<<js["groupid"]<<"] "<<js["userid"]<<" said："<<js["msg"]<<endl;
            continue;
        }
    }
}

