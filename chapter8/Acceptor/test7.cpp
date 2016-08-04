//
// Created by zhouyang on 16-8-4.
//
#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include <stdio.h>

void newConnection(int sockfd,const muduo::InetAddress &peerAddr){
    printf("new Connection() accpted a new connection from %s \n",peerAddr.toHostPort().c_str());
    ::write(sockfd,"how are you?\n",13);
    muduo::sockets::close(sockfd);

}

int main(){
    printf("main(): pid= %d\n",getpid());
    muduo::InetAddress listenAddr(9981);
    muduo::EventLoop loop;
    muduo::Acceptor acceptor(&loop,listenAddr);
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();
    loop.loop();
}