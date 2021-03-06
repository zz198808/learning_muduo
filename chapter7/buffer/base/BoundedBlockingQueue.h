//
// Created by fit on 16-8-23.
//

#ifndef BOUNDEDBLOCKINGQUEUE_H
#define BOUNDEDBLOCKINGQUEUE_H

#include "Condition.h"
#include "Mutex.h"
#include <boost/circular_buffer.hpp>
#include <boost/noncopyable.hpp>
#include <assert.h>

namespace muduo {
    template<typename T>
    class BoundedBlockingQueue : boost::noncopyable {
    public:
        BoundedBlockingQueue(int maxSize) :
                mutex_(),
                notEmpty_(mutex_),
                notFull_(mutex_),
                queue_(maxSize) {}

        //put x into queue_
        void put(const T &x) {
            MutexLockGuard lock(mutex_);
            while (queue_.full()) {
                notFull_.wait();
            }
            assert(!queue_.full());
            queue_.push_back(x);
            notEmpty_.notify();
        }

        //右值引用版本
        void put(const T &&x) {
            MutexLockGuard lock(mutex_);
            while (queue_.full()) {
                notFull_.wait();
            }
            assert(!queue_.full());
            queue_.push_back(std::move(x));
            notEmpty_.notify();
        }

        //return front elem of queue_
        T take() {
            MutexLockGuard lock(mutex_);
            while (queue_.empty()) {
                notEmpty_.wait();
            }
            assert(!queue_.empty());
            T front(std::move(queue_.front()));
            queue_.pop_front();
            notFull_.notify();
            return front;
        }

        bool empty() const {
            MutexLockGuard lock(mutex_);
            return queue_.empty();
        }

        bool full() const {
            MutexLockGuard lock(mutex_);
            return queue_.full();
        }

        ssize_t size() const {
            MutexLockGuard lock(mutex_);
            return queue_.size();
        }

        ssize_t capacity() const {
            MutexLockGuard lock(mutex_);
            return queue_.capacity();
        }

    private:
        mutable MutexLock mutex_;
        //notify thread when queue_ is not empty
        Condition notEmpty_;
        //notify thread when queue_ is not full
        Condition notFull_;
        boost::circular_buffer<T> queue_;
    };


}

#endif
