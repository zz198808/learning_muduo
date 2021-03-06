
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

#include <stdio.h>
//fixme: an bug! double free
using namespace muduo;
using namespace muduo::net;

void onHighWaterMark(const TcpConnectionPtr& conn, size_t len)
{
    LOG_INFO << "HighWaterMark " << len;
}

const int kBufSize = 64*1024;
const char* g_file = NULL;

void onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO << "FileServer - " << conn->peerAddress().toIpPort() << " -> "
             << conn->localAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");
    if (conn->connected())
    {
        LOG_INFO << "FileServer - Sending file " << g_file
                 << " to " << conn->peerAddress().toIpPort();
        conn->setHighWaterMarkCallback(onHighWaterMark, kBufSize+1);

        FILE* fp = ::fopen(g_file, "rb");
        if (fp)
        {
            conn->setContext(fp);
            char buf[kBufSize];
            size_t nread = ::fread(buf, 1, sizeof buf, fp);
            conn->send(buf, static_cast<int>(nread));
        }
        else
        {
            conn->shutdown();
            LOG_INFO << "FileServer - no such file";
        }
    }
    else
    {
        if (!conn->getContext().empty())
        {
            FILE* fp = boost::any_cast<FILE*>(conn->getContext());
            if (fp)
            {
                ::fclose(fp);
            }
        }
    }
}

void onWriteComplete(const TcpConnectionPtr& conn)
{
    FILE* fp = boost::any_cast<FILE*>(conn->getContext());
    char buf[kBufSize];
    size_t nread = ::fread(buf, 1, sizeof buf, fp);
    if (nread > 0)
    {
        conn->send(buf, static_cast<int>(nread));
    }
    else
    {
        ::fclose(fp);
        fp = NULL;
        conn->setContext(fp);
        conn->shutdown();
        LOG_INFO << "FileServer - done";
    }
}

int main() {
    LOG_INFO << " pid= " << getpid();
    g_file = "/home/zhouyang/file";
    EventLoop loop;
    InetAddress listenAddr(2016);
    TcpServer server(&loop, listenAddr, "FileServer");
    //set connection callback function
    server.setConnectionCallback(onConnection);
    //set write callback function
    server.setWriteCompleteCallback(onWriteComplete);
    //start server
    server.start();
    //begin event loop
    loop.loop();
}