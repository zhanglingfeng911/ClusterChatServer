#ifndef PUBLIC_H
#define PUBLIC_H


enum EnMsgType
{
    LOGIN_MSG=1,// 1 登陆消息
    LOGIN_MSG_ACK,//2 登陆消息回应
    LOGINOUT_MSG, // 3注销消息
    REG_MSG,//4 注册消息
    REG_MSG_ACK,//5 注册消息回应
    ONE_CHAT_MSG,//6  1对1聊天消息
    ADD_FRIEN_MSG,//7  添加好友消息

    CREATE_GROUP_MSG,//8 创建群组
    ADD_GROUP_MSG,//9 加入群组
    GROUP_CHAT_MSG,//10 群聊天

};



#endif