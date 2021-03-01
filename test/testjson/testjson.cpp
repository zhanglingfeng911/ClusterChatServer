#include"json.hpp"
using json=nlohmann::json;

#include<iostream>
#include<vector>
#include<map>

using namespace std;

//json 序列化1
string func1(){
    json js;
    js["id"]={1,2,3,4,5};
    js["name"]="zhang san";
    js["msg"]["zhang san"]="hello world";
    js["msg"]["liu shuo"]="hello china";
    string sendBuf=js.dump();

    cout<<sendBuf<<endl;
    return sendBuf;

}

//json 序列化2
string func2()
{
    json js;
    vector<int>vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);

    js["list"]=vec;

    map<int,string>m;
    m.insert({1,"黄山"});
    m.insert({2,"华山"});
    m.insert({3,"泰山"});

    js["path"]=m;

    string sendBuf=js.dump();
    cout<<sendBuf<<endl;
    return sendBuf;


}

//json数据反序列化
int main()
{
    // func1();
    // func2();

    string recvBuf=func2();

    json jsbuf=json::parse(recvBuf);

    // cout<<jsbuf["id"]<<endl;
    // cout<<jsbuf["name"]<<endl;
    // cout<<jsbuf["msg"]<<endl;
    map<int,string>mp=jsbuf["path"];
    for(auto &p:mp){

        cout<<p.first<<" "<<p.second<<endl;
    }
    cout<<jsbuf["path"][2]<<endl;
    return 0;
}

