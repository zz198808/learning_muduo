//
// Created by zhouyang on 16/8/1.
//

#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "datetime/Timestamp.h"
#include "Thread.h"
#include "Callbacks.h"
#include "TimerId.h"
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

namespace muduo {
    class Channel;

    class Poller;

    class TimerQueue;

    class EventLoop : boost::noncopyable {
    public:
        EventLoop();

        ~EventLoop();

        void loop();

        void quit();

        //time when poll returns,usually means data arrival.
        Timestamp pollReturnTime() const {
            return pollReturnTime_;
        }

        //run callback at time
        TimerId runAt(const Timestamp &time, const TimerCallback &cb);

        //run callback after delay seconds
        TimerId runAfter(double delay, const TimerCallback &cb);

        // run callback every interval seconds
        TimerId runEvery(double interval, const TimerCallback &cb);

        //internal use only
        void updateChannel(Channel *channel);

        bool isInLoopThread() const {
            return threadId_ == CurrentThread::tid();
        }

        void assertInLoopThread() {
            if (!isInLoopThread())
                abortNotInLoopThread();
        }


    private:
        void abortNotInLoopThread();

        typedef std::vector<Channel *> ChannelList;

        bool looping_;
        //atomic
        bool quit_;
        //atomic
        const pid_t threadId_;
        Timestamp pollReturnTime_;
        boost::scoped_ptr<Poller> poller_;
        boost::scoped_ptr<TimerQueue> timerQueue_;
        ChannelList activeChannels_;
    };


}


#endif //TIMEQUEUE_EVENTLOOP_H