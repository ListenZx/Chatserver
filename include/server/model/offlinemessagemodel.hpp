#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include<vector>
#include<string>
using namespace std;

class OfflineMessageModel
{
public:
    //新增用户的离线消息
    void insert(int id,string msg);

    //删除用户的离线消息
    void remove(int id);

    //查询用户的离线消息
    vector<string> query(int id);
};

#endif