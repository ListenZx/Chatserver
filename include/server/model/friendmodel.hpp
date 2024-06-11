#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include<vector>
#include"user.hpp"
class FriendModel
{
public:
    //添加好友关系
    void insert(int id,int friendid);
    //删除好友关系
    void remove(int id,int friendid);
    //返回用户好友关系 做联合查询
    vector<User> query(int id);

};

#endif