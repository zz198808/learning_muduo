//
// Created by zhouyang on 16-7-29.
//

#include "EventLoop.h"

#include "Channel.h"
#include "Poller.h"

#include <assert.h>

#include <stdio.h>

using namespace muduo;

__thread EventLoop *t_loopInThisThread = 0;
const int kPollTimeMs = 10000;

EventLoop::EventLoop()
        : looping_(false), quit_(false),
          threadId_(CurrentThread::tid()),
          poller_(new Poller(this)) {
    printf("EventLoop created %p in thread %u \n", this, threadId_);
    if (t_loopInThisThread) {
        printf("Another EventLoop %p exists in this thread %u\n", t_loopInThisThread, threadId_);
    } else
        t_loopInThisThread = this;
}

EventLoop::~EventLoop() {
    assert(!looping_);
    t_loopInThisThread = NULL;
}

void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    while (!quit_) {
        activeChannels_.clear();
        //获取当前活动事件的Channel列表
        poller_->poll(kPollTimeMs, &activeChannels_);
        for (ChannelList::iterator it = activeChannels_.begin();
             it != activeChannels_.end(); it++) {
            //handle occured events
            (*it)->handleEvent();
        }
        printf("EventLoop %p stop this looping\n", this);
        looping_ = false;
    }
}

//if you want to stop loop, set quit_ to true
void EventLoop::quit() {
    quit_ = true;
}

void EventLoop::updateChannel(Channel *channel){
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::abortNotInLoopThread() {
    printf("EventLoop::abortNotInLoopThread - EventLoop %p was created in threadId_ = %u,current thread id = %u \n",
           this, threadId_, CurrentThread::tid());
    abort();
}