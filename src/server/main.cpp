#include <iostream>
#include<signal.h>
#include "chatserver.hpp"
#include"chatservice.hpp"
using namespace std;

//处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int ){
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc,char**argv)
{
    if(argc<3){
        cerr<<"command invalid! example: ./ChatServer 127.0.0.1 6000"<<endl;
        exit(-1);
    }
    //通过解析命令行参数传递的ip和port
    char* ip=argv[1];
    uint16_t port=atoi(argv[2]);

    //捕获通过ctrl+c退出服务器的信号
    //sighandler_t signal(int signum, sighandler_t handler);
    signal(SIGINT,resetHandler);
    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop(); //开启事件循环
    
    return 0;
}