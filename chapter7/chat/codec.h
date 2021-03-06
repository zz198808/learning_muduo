//
// Created by zhouyang on 16-7-24.
//

#ifndef CHAT_CODEC_H_H
#define CHAT_CODEC_H_H

#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Endian.h>
#include <muduo/net/TcpConnection.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

class LengthHeaderCodec : boost::noncopyable {
public:
    //定义函数模板
    typedef boost::function<void(const muduo::net::TcpConnectionPtr &, const muduo::string &message,
                                 muduo::Timestamp)> StringMessageCallback;

    //设置回调函数
    explicit LengthHeaderCodec(const StringMessageCallback &cb)
            : messageCallback_(cb) {

    }

    //服务器处理消息时会调用此函数
    void onMessage(const muduo::net::TcpConnectionPtr &con, muduo::net::Buffer *buf, muduo::Timestamp receiveTime) {
        //可读的数据长度大于头的长度
        while (buf->readableBytes() >= kHeaderLen) {
            //获取数据长度标志
            const void *data = buf->peek();
            int32_t be32 = *static_cast<const int32_t *>(data);
            const int32_t len = muduo::net::sockets::networkToHost32(be32);
            //长度不符合规则
            if (len > 65536 || len < 0) {
                LOG_ERROR << "Invalid length " << len;
                con->shutdown();
                break;
            }
                //可读的数据长度大于一个数据报，读取这个数据报，并调用消息回调函数
            else if (buf->readableBytes() >= len + kHeaderLen) {
                buf->retrieve(kHeaderLen);
                muduo::string message(buf->peek(), len);
                //call message callback function in ChatServer
                messageCallback_(con, message, receiveTime);
                //set read index to right pos
                buf->retrieve(len);
            }
            else
                break;
        }
    }
    //发送数据会调用此函数，自动在数据报前面加上消息长度标志，并发送消息
    void send(muduo::net::TcpConnection *con, const muduo::StringPiece &message) {
        muduo::net::Buffer buf;
        buf.append(message.data(), message.size());
        int32_t len = static_cast<int32_t >(message.size());
        int32_t be32 = muduo::net::sockets::hostToNetwork32(len);
        //将buffer中数据的长度附在buffer的头部位置
        buf.prepend(&be32, sizeof be32);
        //发送数据
        con->send(&buf);
    }

private:
    StringMessageCallback messageCallback_;
    //消息头长度
    const static size_t kHeaderLen = sizeof(int32_t);
};


#endif //CHAT_CODEC_H_H
