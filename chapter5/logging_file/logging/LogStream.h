//
// Created by fit on 16-8-6.
//

#ifndef LOGSTREAM_H
#define LOGSTREAM_H

#include <assert.h>
#include <string.h>
#include <string>
#include <boost/noncopyable.hpp>

namespace muduo {
    using std::string;
    namespace detail {
        const int kSmallBuffer = 4000;
        const int kLargeBuffer = 4000 * 1000;

        template<int SIZE>
        class FixedBuffer : boost::noncopyable {
        public:
            FixedBuffer() : cur_(data_) {
                setCookie(cookieStart);
            }

            ~FixedBuffer() {
                setCookie(cookieEnd);
            }

            //append buf to cur_
            void append(const char *buf, int len) {
                if (avail() > len) {
                    memcpy(cur_, buf, len);
                    cur_ += len;
                }
            }

            const char *data() const {
                return data_;
            }

            //return length
            int length() const {
                return cur_ - data_;
            }

            //return character to current pointer
            char *current() {
                return cur_;
            }

            //capacity remain
            int avail() {
                return static_cast<int>(end() - cur_);
            }

            //forward cur_ pointer len pos
            void add(size_t len) {
                cur_ += len;
            }

            //reset pointer to head of data_
            void reset() {
                cur_ = data_;
            }


            //same as memset function
            void bzero() {
                // Set N bytes of S to 0.
                ::bzero(data_, sizeof data_);
            }

            //for used by gdb
            const char *debugString();

            void setCookie(void(*cookie)()) {
                cookie_ = cookie;
            }

            //for used for unit test
            string asString() const {
                return string(data_, length());
            }

        private:
            //return end charcter
            const char *end() const {
                return data_ + sizeof data_;
            }

            static void cookieStart();

            static void cookieEnd();

            void (*cookie_)();

            char data_[SIZE];
            char *cur_;
        };
    }

    //helper class for known string length at compile time
    class T {
    public:
        T(const char *str, int len) : str_(str), len_(len) {

           assert(strlen(str) == len_);
        }

        const char *str_;
        const size_t len_;
    };

    class LogStream : boost::noncopyable {
        typedef LogStream self;
    public:
        typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

        self &operator<<(bool v) {
            buffer_.append(v ? "1" : "0", 1);
            return *this;
        }

        self &operator<<(short);

        self &operator<<(unsigned short);

        self &operator<<(int);

        self &operator<<(unsigned int);

        self &operator<<(long);

        self &operator<<(long long);

        self &operator<<(unsigned long);

        self &operator<<(unsigned long long);

        self &operator<<(const void *);

        self &operator<<(float v) {
            *this << static_cast<double>(v);
            return *this;
        }

        self &operator<<(double);

        self &operator<<(char v) {
            buffer_.append(&v, 1);
            return *this;
        }

        self &operator<<(const char *v) {
            buffer_.append(v, strlen(v));
            return *this;
        }

        self &operator<<(const T &v) {
            buffer_.append(v.str_, v.len_);
            return *this;
        }

        self &operator<<(const string &v) {
            buffer_.append(v.c_str(), v.size());
            return *this;
        }

        void append(const char *data, int len) {
            buffer_.append(data, len);
        }

        const Buffer &buffer() const {
            return buffer_;
        }

        void resetBuffer() {
            buffer_.reset();
        }

    private:
        void staticCheck();

        template<typename T>
        void formatInteger(T);

        Buffer buffer_;

        static const int kMaxNumericSize = 32;

    };

    class Fmt {
    public:
        template<typename T>
        Fmt(const char *fmt, T val);

        const char *data() const {
            return buf_;
        }

        int length() const {
            return length_;
        }

    private:
        char buf_[32];
        int length_;
    };

    inline LogStream &operator<<(LogStream &s, const Fmt &fmt) {
        s.append(fmt.data(), fmt.length());
        return s;
    }

}


#endif //LOGSTREAM_H
