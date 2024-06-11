#include"groupmodel.hpp"
#include"db.h"
//创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024]={};
    sprintf(sql,"insert into AllGroup(groupname,groupdesc) values('%s','%s')",group.getName().c_str(),group.getDesc().c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            //mysql_insert_id()：这通常是用于获取在 MySQL 数据库中执行上一次插入操作后自动生成的主键值（自增主键的值）。
            //mysql.getConnection()：这是获取与 MySQL 数据库的连接对象。
            return true;
        }
    }
    return false;
}
//加入群组
bool GroupModel::addGroup(int userid,int groupid,string role)
{
    char sql[128]={0};
    sprintf(sql,"insert into GroupUser values(%d,%d,'%s')",groupid,userid,role.c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}
//查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    /*
    1, 先根据userid在groupuser表中查询出该用户所属的群组信息
    2，在根据群组信息去查询属于该群组的所有用户的userid，并且和user表进行多表联合查询，查出用户的详细信息
    */
    char sql[128]={0};
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from AllGroup a inner join GroupUser b on a.id = b.groupid where b.userid=%d",userid);
    vector<Group> groupVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res=mysql.query(sql);
        if(res!=nullptr)
        {
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                Group group(stoi(row[0]),row[1],row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    for(Group &group:groupVec)
    {
        sprintf(sql,"select a.id,a.name,a.state,b.grouprole from User a inner join GroupUser b on a.id=b.userid where b.groupid=%d",group.getId());
        MYSQL_RES *res=mysql.query(sql);
        if(sql!=nullptr)
        {
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                GroupUser user;
                user.setId(stoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsersVec().push_back(user);
            }
        }
        mysql_free_result(res);
    }
    return groupVec;
}
//根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务：给群组其他成员（群）发消息
vector<int> GroupModel::queryGroupUsers(int userid,int groupid)
{
    char sql[128]={0};
    sprintf(sql,"select userid from GroupUser where userid!=%d and groupid=%d",userid,groupid);
    vector<int>idVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res=mysql.query(sql);
        if(res!=nullptr)
        {
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                idVec.push_back(stoi(row[0]));
            }
        }
    }
    return idVec;
}