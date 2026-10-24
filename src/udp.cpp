#include "udp.h"

#include <cerrno>
#include <cstring>
#include <string>

#include <sys/socket.h>
#include <sys/time.h>

#include "socket.h"


// some services ignore empty packets, so these get a real request
static std::string udp_payload(int port) {
    if (port == 53) {
        // DNS query for the root name servers
        const char dns[] = {0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01};
        return std::string(dns, sizeof(dns));
    }

    if (port == 123) {
        // NTP client request, version 3
        std::string ntp(48, '\0');
        ntp[0] = 0x1b;
        return ntp;
    }

    return "";
}


// reply = open, ICMP unreachable = closed, nothing = open|filtered
PortResult scan_udp_port(const sockaddr_in& target, int timeout_max) {
    Socket sock(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));

    if (!sock.valid()) {
        return {PortStatus::Error, std::string("socket: ") + strerror(errno)};
    }

    int s = sock.fd();

    // UDP connect sends nothing, but ICMP errors now come back as ECONNREFUSED
    if (connect(s, reinterpret_cast<const sockaddr*>(&target), sizeof(target)) == -1) {
        return {PortStatus::Error, std::string("connect: ") + strerror(errno)};
    }

    timeval tv{};
    tv.tv_sec = timeout_max;

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::string payload = udp_payload(ntohs(target.sin_port));
    send(s, payload.data(), payload.size(), 0);

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = recv(s, buffer, BUFFER_SIZE, 0);

    if (bytesRead >= 0) {
        return {PortStatus::Open, "(" + std::to_string(bytesRead) + " byte reply)"};
    }

    switch (errno) {
        case ECONNREFUSED:
            return {PortStatus::Closed, ""};

        case EAGAIN:   // same as EWOULDBLOCK on Linux
            return {PortStatus::Filtered, ""};

        default:
            return {PortStatus::Error, strerror(errno)};
    }
}
