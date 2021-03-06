//
// Created by fit on 16-10-14.
//

#ifndef S11_CONNECTOR_H
#define S11_CONNECTOR_H

#include "InetAddress.h"
#include "TimerId.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

//这个类只是用于建立连接，连接完毕之后，注销channel，使用连接得到的sockfd调用回调函数
namespace muduo {
    class Channel;
    class EventLoop;

    class Connector : boost::noncopyable {
    public:
        /// new connection callback function
        typedef boost::function<void(int sockfd)> NewConnectionCallback;

        Connector(EventLoop* loop, const InetAddress& serverAddr);

        ~Connector();

        //set new connection callback function
        void setNewConnectionCallback(const NewConnectionCallback & cb)
        {
            newConnectionCallback_ = cb;
        }

        //can be called in any thread
        void start();
        //must be called in loop thread
        void restart();
        //can be called in any thread
        void stop();

        const InetAddress& serverAddress() const
        {
            return serverAddr_;
        }

    private:

        enum States {
            kDisconnected, kConnecting, kConnected
        };

        static const int kMaxRetryDelayMs = 30*1000;
        static const int kInitRetryDelayMs = 500;

        void setState(States s)
        {
            state_ = s;
        }

        void startInLoop();

        void connect();

        void connecting(int sockfd);

        void handleWrite();

        void handleError();

        void retry(int sockfd);

        int removeAndResetChannel();

        void resetChannel();

        EventLoop* loop_;

        InetAddress serverAddr_;
        bool connect_;
        States state_;
        boost::scoped_ptr<Channel> channel_;
        NewConnectionCallback newConnectionCallback_;
        //控制重连时间间隔
        int retryDelayMs_;
        TimerId timerId_;

    };

    typedef boost::shared_ptr<Connector> ConnectorPtr;
}

#endif //S11_CONNECTOR_H
