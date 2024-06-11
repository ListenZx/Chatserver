#include"friendmodel.hpp"
#include"db.h"
#include<iostream>
#include<string>
using namespace std;
void FriendModel::insert(int id,int friendid)
{
    char sql[128]={0};
    sprintf(sql,"insert into Friend value(%d,%d)",id,friendid);
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
            return;
    }
}

void FriendModel::remove(int id,int friendid)
{
    char sql[128]={0};
    sprintf(sql,"delete friend Friend where userid=%d and frindid=%d or frindid=%d and userid=%d",id,friendid,friendid,id);
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
            return;
    }
}

vector<User> FriendModel::query(int id)
{
    char sql[1000]={0};
    sprintf(sql,"SELECT DISTINCT * FROM ((select a.id,a.name,a.state from User a inner join Friend b on b.friendid=a.id where b.userid=%d) UNION ALL (select a.id,a.name,a.state from User a inner join Friend b on b.userid=a.id where b.friendid=%d))t",id,id);
    MySQL mysql;
    vector<User>vec;
    if(mysql.connect())
    {
        MYSQL_RES *res=mysql.query(sql);
        if(res!=nullptr)
        {
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                User user;
                user.setId(stoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user); //存储副本
            }
            mysql_free_result(res);
        }
    }
    return vec;
}
