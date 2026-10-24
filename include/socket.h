#pragma once

#include <unistd.h>


// Closes the socket when it goes out of scope
class Socket {
public:
    explicit Socket(int fd) : fd_(fd) {}

    ~Socket() {
        if (fd_ != -1) {
            close(fd_);
        }
    }

    // no copies, both would close the same fd
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    int fd() const { return fd_; }
    bool valid() const { return fd_ != -1; }

private:
    int fd_;
};
