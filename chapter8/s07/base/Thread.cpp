//
// Created by fit on 16-8-24.
//

#include "Thread.h"
#include "CurrentThread.h"
#include "Exception.h"
#include "Logging.h"

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/weak_ptr.hpp>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo {
    namespace CurrentThread {
        __thread int t_cachedTid = 0;
        __thread char t_tidString[32];
        __thread int t_tidStringLength = 6;
        __thread const char *t_threadName = "unknown";
        const bool sameType = boost::is_same<int, pid_t>::value;
        // assert pid_t type is int
        BOOST_STATIC_ASSERT(sameType);
    }
    namespace detail {
        //get thread tid
        pid_t gettid() {
            return static_cast<pid_t>(::syscall(SYS_gettid));
        }

        void afterFork() {
            muduo::CurrentThread::t_cachedTid = 0;
            muduo::CurrentThread::t_threadName = "main";
            CurrentThread::tid();
            // no need to call pthread_atfork(NULL,NULL,&afterFork);
        }

        class ThreadNameInitializer {
        public:
            ThreadNameInitializer() {
                muduo::CurrentThread::t_threadName = "main";
                CurrentThread::tid();
                //call afterFork function after fork
                pthread_atfork(NULL, NULL, &afterFork);
            }
        };

        ThreadNameInitializer init;

        struct ThreadData {
            typedef muduo::Thread::ThreadFunc ThreadFunc;
            ThreadFunc func_;
            string name_;
            boost::weak_ptr<pid_t> wkTid_;

            ThreadData(const ThreadFunc &func, const string &name, const boost::shared_ptr<pid_t> &tid) :
                    func_(func), name_(name), wkTid_(tid) {}

            //run this function in the thread
            void runInThread() {
                pid_t tid = muduo::CurrentThread::tid();
                boost::shared_ptr<pid_t> ptid = wkTid_.lock();
                //set thread tid
                if (ptid) {
                    *ptid = tid;
                    //减少ptid的引用计数
                    ptid.reset();
                }
                muduo::CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
                //PR_SET_NAME :把参数arg2作为调用进程的名字
                ::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);
                try {
                    //call function
                    func_();
                    muduo::CurrentThread::t_threadName = "finished";
                } catch (const Exception &ex) {
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                    fprintf(stderr, "reason:%s\n", ex.what());
                    fprintf(stderr, "stack trace: %s \n", ex.stackTrace());
                    abort();
                }
                catch (const std::exception &ex) {
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                    fprintf(stderr, "reason:%s\n", ex.what());
                    abort();
                }
                catch (...) {
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "unknown exception caught in Thread %s \n", name_.c_str());
                    throw;
                }
            }
        };

        void *startThread(void *obj) {
            //function run after start thread
            ThreadData *data = static_cast<ThreadData *>(obj);
            data->runInThread();
            delete data;
            return NULL;
        }

    }
}
using namespace muduo;

void CurrentThread::cacheTid() {
    if (t_cachedTid == 0) {
        t_cachedTid = detail::gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
    }
}

bool CurrentThread::isMainThread() {
    return tid() == ::getpid();
}

void CurrentThread::sleepUsec(int64_t usec) {
    struct timespec ts = {0, 0};
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
    ::nanosleep(&ts, NULL);
}

AtomicInt32 Thread::numCreated_;

Thread::Thread(const ThreadFunc &func, const string &n)
        : started_(false), joined_(false), pthreadId_(0), tid_(new pid_t(0)), func_(func), name_(n) {
    setDefaultName();
}

Thread::Thread(ThreadFunc &&func, const string &n)
        : started_(false), joined_(false), pthreadId_(0), tid_(new pid_t(0)), func_(std::move(func)), name_(n) {
    setDefaultName();
}

Thread::~Thread() {
    if (started_ && !joined_) {
        pthread_detach(pthreadId_);
    }
}

void Thread::setDefaultName() {
    int num = numCreated_.incrementAndGet();
    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}

//start the thread
void Thread::start() {
    assert(!started_);
    started_ = true;
    detail::ThreadData *data = new detail::ThreadData(func_, name_, tid_);
    //call startThread function in thread
    if (pthread_create(&pthreadId_, NULL, &detail::startThread, data)) {
        started_ = false;
        delete data;
        LOG_SYSFATAL << "Failed in pthread_create";
    }
}

int Thread::join() {
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}