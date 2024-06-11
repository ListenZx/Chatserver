#ifndef GROUP_H
#define GROUP_H
#include<string>
#include<vector>
using namespace std;
#include"groupuser.hpp"

//group表的ORM类
class Group
{
public:
    Group(int id=-1,string name="",string desc="",vector<GroupUser>usersVec={})
    :id(id),name(name),desc(desc),usersVec(usersVec){}

    void setId(int id){this->id=id;}
    void setName(string name){this->name=name;}
    void setDesc(string desc){this->desc=desc;}
    void setUsersVec(vector<GroupUser>usersVec){this->usersVec=usersVec;}

    int getId(){return this->id;}
    string getName(){return this->name;}
    string getDesc(){return this->desc;}
    vector<GroupUser>& getUsersVec(){return this->usersVec;}
private:
    int id;
    string name;
    string desc;
    vector<GroupUser>usersVec; //记录群组成成员信息
};

#endif