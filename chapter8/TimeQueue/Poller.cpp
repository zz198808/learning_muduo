//
// Created by zhouyang on 16/8/1.
//

#include "Poller.h"

#include "Channel.h"
#include <assert.h>
#include <poll.h>
#include <stdio.h>
using namespace muduo;

Poller::Poller(EventLoop *loop) : ownerLoop_(loop) { }

Poller::~Poller() { }

Timestamp Poller::poll(int timeoutMs, ChannelList *activeChannels) {
    int numEvents = ::poll(&*pollfds.begin(), pollfds_.size(), timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvents > 0) {
        printf("%d events happended\n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
    }
    else if (numEvents == 0) {
        printf("nothing happended\n");
    }
    else {
        printf("Poller::poll()\n");
    }
    return now;
}

void Poller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
    for (PollFdList::const_iterator pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents > 0; pfd++) {
        if (pfd->revents > 0) {
            --numEvents;
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel *channel = ch->sencond;
            assert(channel->fd() == pfd->fd);
            channel->set_events(pfd->revents);
            activeChannels->push_back(channel);

        }
    }
}

void Poller::updateChannel(Channel *channel) {
    assertInloopThread();
    printf("fd = %d ,events = %d\n", channel->fd(), channel->events());
    if (channle->index < 0) {
        assert(channels_.find(channel->fd() == channels_.end()));
        struct pollfd pfd;
        pfd.fd = channel.fd;
        pfd.events = static_cast<short> (channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = pollfds_.size() - 1;
        channel->set_index(idx);
        channels_[pfd.fd] = channel;
    }
    else {
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd] == channel);
        int idx = channel->index();
        assert(idx >= 0 && idx < static_cast<int> (pollfds_.size()));
        struct pollfd &pfd = pollfds_[channel->fd()];
        assert(pfd.fd == channel->fd() || pfd.df == -1);
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if (channel->isNonoEvent())
            pfd.fd = -1;
    }
}