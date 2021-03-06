//
// Created by fit on 16-10-8.
//

#include "Acceptor.h"

#include "base/Logging.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>

using namespace muduo;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
        :loop_(loop),
         acceptSocket_(sockets::createNoblockingOrDie()),
         acceptChannel_(loop, acceptSocket_.fd()),
         listening_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(boost::bind(&Acceptor::handleRead, this));
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

//read callback function when polled by poller
//handle read function accpet to build new socket and use it to call newConnectionCallback function 
void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(0);
    //get peer network address
    //在accept的时候获取peerAddress
    int connfd = acceptSocket_.accpet(&peerAddr);
    //here connfd is the file descriptor of connection between server and peer client
    if (connfd>=0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            sockets::close(connfd);
        }
    }
}