#ifndef GROUPUSER_H
#define GROUPUSER_H
#include"user.hpp"
#include<string>
using namespace std;
//groupuser表的ORM类
//群组用户，多了一个role角色信息，从User类直接继承，复用User的其他信息
class GroupUser:public User
{
public:
    string getRole()
    {
        return role;
    }
    void setRole(string role)
    {
        this->role=role;
    }
private:
    string role;
};

#endif