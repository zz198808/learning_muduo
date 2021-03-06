//
// Created by fit on 16-8-10.
//

#ifndef LOGFILE_H
#define LOGFILE_H

#include "../thread/Mutex.h"
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
namespace muduo {

    using std::string;

    class LogFile : boost::noncopyable {
    public:
        LogFile(const string &basename, size_t rollSize, bool threadSafe = false, int flushInterval = 3);

        ~LogFile();

        void append(const char *logline, int len);

        void flush();

    private:
        void append_unlocked(const char *logline, int len);

        static string getLogFileName(const string &basename, time_t *now);

        void rollFile();

        //basename of logfile
        const string basename_;
        //roll size
        const size_t rollSize_;
        //flush interval
        const int flushInterval_;

        int count_;

        boost::scoped_ptr<MutexLock> mutex_;
        time_t startOfPeriod;
        //上一次roll的时间
        time_t lastRoll_;
        //上一次flush的时间
        time_t lastFlush_;

        class File;

        boost::scoped_ptr<File> file_;

        const static int kCheckTimeRoll_ = 1024;
        const static int kRollPerSeconds_ = 60 * 60 * 24;

    };

}


#endif //LOGGING_FILE_LOGFILE_H
