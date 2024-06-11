#include"offlinemessagemodel.hpp"
#include"db.h"
#include"user.hpp"

//新增用户的离线消息
void OfflineMessageModel::insert(int id,string msg)
{   
    //1.组装
    char sql[128]={0};
    sprintf(sql,"insert into OfflineMessage values(%d,'%s')",id,msg.c_str());

    //2.连接mysql
    MySQL mysql;
    if(mysql.connect())
    {   
        //3,发送数据库语句 
        if(mysql.update(sql))
        {
            return;
        }
    }
}

//删除用户的离线消息
void OfflineMessageModel::remove(int id)
{
    //1,组装
    char sql[128]={0};
    sprintf(sql,"delete from OfflineMessage where userid=%d",id);

    //2,连接数据库
    MySQL mysql;
    if(mysql.connect())
    {
        //3,发送数据库语句
        if(mysql.update(sql))
        {
            return;
        }
    }
}

//查询用户的离线消息
vector<string> OfflineMessageModel::query(int id)
{
    vector<string>vec;
    //1,组装
    char sql[128]={0};
    sprintf(sql,"select message from OfflineMessage where userid=%d",id);

    //2,连接数据库
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res=mysql.query(sql);
        if(res!=nullptr)
        {
            //把userid用户所有离线消息放到vec中返回
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res); 
        }
    }
    return vec;
}