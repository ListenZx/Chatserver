#include"usermodel.hpp"
#include<iostream>
#include"db.h"

#include<muduo/base/Logging.h> //日志库

//user表的增加方法
bool UserModel::insert(User &user)
{
    //1,组装mysql语句
    char sql[1024]={};
    
    sprintf(sql,"insert into User(name,password,state) values('%s','%s','%s')",user.getName().c_str(),user.getPwd().c_str(),user.getState().c_str());
    //printf(const char *format, ...)根据用户提供的数据拼接成一条完整的 SQL 插入语句，然后将这条 SQL 语句存储在 sql 变量中
    //存在 SQL 注入的安全风险。SQL 注入是一种攻击方式，攻击者通过在输入中注入恶意的 SQL 代码，从而破坏数据库查询，甚至获取敏感数据。

    //2，连接数据库
    MySQL mysql;
    if(mysql.connect())
    {   
        //3,发送数据库语句 查看zhu
        if(mysql.update(sql))
        {
            //获取插入成功的用户数据生成的主键id
            //mysql提供了很多方法
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
    
}

//根据用户号码查询用户信息
User UserModel::queryid(int id)
{
    char sql[1024]={};
    sprintf(sql,"select * from User where id=%d",id);

    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res=mysql.query(sql); //mysql.query（）返回是MYSQL_RES类型
        if(res!=nullptr) //查询成功
        {
            //查询成功的数据
            MYSQL_ROW row=mysql_fetch_row(res);
            if(row!=nullptr) //成功查询到数据
            {
                User user;
                user.setId(stoi(row[0])); //atoi:all to int
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res); //释放资源：否则不断泄漏内存
                return user;
            } 
        }
    }
    return User();//默认 id==-1 表示出错
}

//更新用户的状态信息
bool UserModel::updateState(User &user)
{
    char sql[1024]={};
    sprintf(sql,"update User set state='%s' where id=%d",user.getState().c_str(),user.getId());

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

//重置用户的状态信息
void  UserModel::resetState()
{
    char sql[128]={0};
    sprintf(sql,"update User set state='offline' where state='online'");
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
            return;
    }
    return;
}