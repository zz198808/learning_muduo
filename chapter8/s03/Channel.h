//
// Created by zhouyang on 16-9-27.
//

#ifndef S02_CHANNEL_H
#define S02_CHANNEL_H
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>


namespace muduo{

    class EventLoop;

    /// A selectable I/O channel

    //this class doesn't own the file descriptor
    //the file descriptor could be a socket
    //an eventfd,a timefd, or signalfd

    class Channel:boost::noncopyable{
    public:
        typedef boost::function<void()> EventCallback;

        Channel(EventLoop * loop,int fd);


        //handle event that occurred
        void handleEvent();

        void setReadCallback(const EventCallback &cb){
            readCallback_ = cb;
        }

        void setWriteCallback(const EventCallback &cb){
            writeCallback_ = cb;
        }

        void setErrorCallback(const EventCallback &cb){
            errorCallback_ = cb;
        }

        int fd() const{
            return fd_;
        }

        int events() const{
            return events_;
        }

        void set_revents(int revt){
            revents_ = revt;
        }

        bool isNoneEvent() const{
            return events_ ==kNoneEvent;
        }

        void enableReading(){
            events_ |= kReadEvent;
            update();
        }

        void enableWriting(){
            events_|=kWriteEvent;
            update();
        }

        int index(){
            return index_;
        }
        void set_index(int idx){
            index_ = idx;
        }
        EventLoop * ownerLoop(){
            return loop_;
        }

    private:
        void update();

        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;

        //pointer of EventLoop
        EventLoop * loop_;
        const int fd_;
        //events that cares about
        int events_;
        //events return that occurred
        int revents_;
        //index in poller
        int index_;

        //read callback function
        EventCallback readCallback_;
        //write callback function
        EventCallback writeCallback_;
        //error callback function
        EventCallback errorCallback_;

    };

}



#endif //S02_CHANNEL_H
