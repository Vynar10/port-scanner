#include "tcp.h"

#include <cerrno>
#include <cstring>
#include <string>

#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <poll.h>

#include "socket.h"


static std::string grab_banner(int s, int timeout_max) {
    timeval tv{};
    tv.tv_sec = timeout_max;

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = recv(s, buffer, BUFFER_SIZE, 0);

    // silent service: ask it something, then read again
    if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        const char probe[] = "HEAD / HTTP/1.0\r\n\r\n";

        send(s, probe, sizeof(probe) - 1, MSG_NOSIGNAL);
        bytesRead = recv(s, buffer, BUFFER_SIZE, 0);
    }

    if (bytesRead > 0) {
        std::string banner(buffer, bytesRead);
        banner.erase(0, banner.find_first_not_of("\r\n"));   // drop leading line breaks
        banner.erase(banner.find_first_of("\r\n"));          // keep only the first line

        return banner.empty() ? "(no banner)" : banner;

    } else if (bytesRead == 0) {
        return "(closed by server)";

    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return "(no banner)";

    } else {
        return std::string("(read error: ") + strerror(errno) + ")";
    }
}


// on localhost the source port can match the target port,
// then the socket connects to itself
static bool is_self_connect(int s, const sockaddr_in& target) {
    sockaddr_in local{};
    socklen_t len = sizeof(local);

    if (getsockname(s, reinterpret_cast<sockaddr*>(&local), &len) == -1) {
        return false;
    }

    return local.sin_port == target.sin_port
        && local.sin_addr.s_addr == target.sin_addr.s_addr;
}


PortResult scan_tcp_port(const sockaddr_in& target, int timeout_max) {
    Socket sock(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));

    if (!sock.valid()) {
        return {PortStatus::Error, std::string("socket: ") + strerror(errno)};
    }

    int s = sock.fd();

    /* Making the socket non-blocking */
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

    /* When it starts connecting; return instantly */
    int result = connect(s, reinterpret_cast<const sockaddr*>(&target), sizeof(target));

    if (result == -1 && errno == EINPROGRESS) {

        /* Wait until the handshake finishes at most timeout_max seconds */
        pollfd pfd{};
        pfd.fd = s;
        pfd.events = POLLOUT;

        int ready = poll(&pfd, 1, timeout_max * 1000);

        if (ready == 0) {
            errno = ETIMEDOUT;             // filtered

        } else if (ready > 0) {
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            getsockopt(s, SOL_SOCKET, SO_ERROR, &so_error, &len);

            if (so_error == 0) {
                result = 0;                // open
            } else {
                errno = so_error;          // e.g. ECONNREFUSED: closed
            }
        }
    }

    /* Back to blocking, so recv and SO_RCVTIMEO work like before */
    fcntl(s, F_SETFL, flags);

    if (result == 0) {
        if (is_self_connect(s, target)) {
            return {PortStatus::Closed, ""};
        }

        return {PortStatus::Open, grab_banner(s, timeout_max)};
    }

    switch (errno) {
        case ETIMEDOUT:
            return {PortStatus::Filtered, ""};

        case ECONNREFUSED:
            return {PortStatus::Closed, ""};

        default:
            return {PortStatus::Error, strerror(errno)};
    }
}
