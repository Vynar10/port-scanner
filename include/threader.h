#pragma once

#include <string>
#include <utility>
#include <vector>

#include <netinet/in.h>


struct OpenPort {
    int port;
    std::string banner;
};


// sorted by port
struct ScanResults {
    std::vector<OpenPort> open;
    std::vector<int> closed;
    std::vector<int> filtered;
    std::vector<std::pair<int, std::string>> errors;   // port, message
};


ScanResults run_scan(const sockaddr_in& target, const std::vector<int>& ports,
                     int timeout_max, bool udp, int workers);
