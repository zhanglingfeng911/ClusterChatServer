#ifndef GROUPMODEL_H
#define GROUPMODEL_H
#include"group.hpp"
#include <string>
#include<vector>

using namespace std;
class GroupModel
{
private:
    /* data */
public:
    //创建群组
    bool createGroup(Group& group);
    //加入群组
    void addGroup(int userid,int groupid,string role);
     //查询目前使用用户所在群组->查询群组中所有用户的详细信息
    vector<Group>queryGroups(int userid);
    //查询群里其他用户的id 以发送群聊消息
    vector<int>queryGroupUsers(int userid,int groupid);

};





#endif