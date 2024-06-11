#ifndef PUBLIC_H
#define PUBLIC_H
/*
server和client的公共文件
*/
enum EnMSgType
{
    MSG_TYPE_MIN=0,

    MSG_TYPE_LOGIN_REQUIRE, //登陆消息
    MSG_TYPE_LOGIN_RESPOND, //登陆确认

    MSG_TYPE_REG_REQUIRE, //注册消息
    MSG_TYPE_REG_RESPOND, //注册确认

    MSG_TYPE_LOGINOUT_REQUIRE, //用户注销请求

    MSG_TYPE_ONE_CHAT,//点对点通信请求

    MSG_TYPE_ADDFRIEND_REQUIRE,//添加好友请求
    //MSG_TYPE_ADDFRIEND_RESPOND,//添加好友回复

    MSG_TYPE_CREATEGROUP_REQUIRE,   //创建群组
    MSG_TYPE_ADDGROUP_REQUIRE,  //加入群组
    MSG_TYPE_GROUP_CHAT,    //群发消息


    MSG_TYPE_MAX=0xfffffff,
};

#endif