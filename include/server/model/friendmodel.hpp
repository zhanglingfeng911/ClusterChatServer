#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include<vector>
#include"user.hpp"

using namespace std;


//维护好友信息的接口方法
class FriendModel
{
private:



public:
    //添加好友关系
    void insert(int userid,int friendid);

    //返回用户好友列表信息  (Friend user表的联合查询)
    vector<User>query(int userid);

    
    
};



#endif
