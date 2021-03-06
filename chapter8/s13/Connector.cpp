//
// Created by fit on 16-10-14.
//

#include "Connector.h"

#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include "base/Logging.h"

#include <boost/bind.hpp>

#include <errno.h>

using namespace muduo;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
        :loop_(loop),
         serverAddr_(serverAddr),
         connect_(false),
         state_(kDisconnected),
         retryDelayMs_(kInitRetryDelayMs)
{
    LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
    LOG_DEBUG << "dtor[" << this << "]";
    ///cancel timer
    loop_->cancel(timerId_);
    assert(!channel_);
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(boost::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_==kDisconnected);
    if (connect_)
    {
        ///connect to server
        connect();
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::connect()
{
    int sockfd = sockets::createNoblockingOrDie();
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddrInet());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            break;

        default:
            LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            // connectErrorCallback_();
            break;
    }
}

//restart to connect
void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

//stop connect
void Connector::stop()
{
    connect_ = false;
    //注销定时器
    loop_->cancel(timerId_);
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    //连接完毕，当write事件发生，调用下述回调函数
    channel_->setWriteCallback(boost::bind(&Connector::handleWrite, this));
    //error发生，调用下述回调函数
    channel_->setErrorCallback(boost::bind(&Connector::handleError, this));
    channel_->enableWriting();
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    loop_->removeChannel(get_pointer(channel_));
    int sockfd = channel_->fd();
    loop_->queueInLoop(boost::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}

void Connector::handleWrite()
{
    LOG_TRACE << "Connector::handleWrite" << state_;
    if (state_==kConnecting)
    {
        //remove channel and get sockfd
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if (err)
        {
            LOG_WARN << "Connector::handleWrite -SO_ERROR = " << err << " " << strerror_tl(err);
            retry(sockfd);
        }
        else if (sockets::isSelfConnect(sockfd))
        {
            LOG_WARN << "Connector::hanleWrite - Self connect";
            retry(sockfd);
        }
        else
        {
            //连接完毕
            setState(kConnected);
            if (connect_)
            {
                //call newConnectionCallback function
                newConnectionCallback_(sockfd);
            }
            else
            {
                sockets::close(sockfd);
            }
        }
    }
    else
    {
        assert(state_==kDisconnected);
    }
}

void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError";
    assert(state_==kConnecting);
    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
    //retry to connect
    retry(sockfd);
}

void Connector::retry(int sockfd)
{
    //首先关闭这个socket
    sockets::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        LOG_INFO << "Connector::retry - Retry connecting to"
                 << serverAddr_.toHostPort()
                 << " in " << retryDelayMs_ << " milliseconds .";
        //注册回调函数，再次尝试连接服务器
        timerId_ = loop_->runAfter(retryDelayMs_/1000.0, boost::bind(&Connector::startInLoop, this));
        retryDelayMs_ = std::min(retryDelayMs_*2, kMaxRetryDelayMs);
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

