#ifndef GROUPMODEL_H
#define FROUPMODEL_H

#include"group.hpp"

//维护群租信息的操作接口方法
class GroupModel
{
public:
    //创建群组
    bool createGroup(Group &group);
    //加入群组
    bool addGroup(int userid,int groupid,string role);
    //查询用户所在群组信息
    vector<Group> queryGroups(int userid);
    //根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务：给群组其他成员（群）发消息
    vector<int> queryGroupUsers(int userid,int groupid); //通过id可以联合connection查询用户连接
};

#endif