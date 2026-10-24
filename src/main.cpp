#include <chrono>
#include <iostream>
#include <optional>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "colors.h"
#include "output.h"
#include "parse.h"
#include "threader.h"


std::optional<sockaddr_in> resolve_host(const std::string& host) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* res = nullptr;
    int err = getaddrinfo(host.c_str(), nullptr, &hints, &res);

    if (err != 0) {
        std::cerr << "Could not resolve " << host << ": " << gai_strerror(err) << std::endl;
        return std::nullopt;
    }

    sockaddr_in sadd{};
    sadd.sin_family = AF_INET;
    sadd.sin_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;

    freeaddrinfo(res);
    return sadd;
}


std::string ip_to_string(const sockaddr_in& addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    return ip;
}


int main(int argc, char* argv[]) {
    Options opts;

    if (auto exit_code = parse_args(argc, argv, opts)) {
        return *exit_code;
    }

    init_colors(opts.color);

    std::optional<sockaddr_in> target = resolve_host(opts.host);

    if (!target) {
        return 1;
    }

    auto start = std::chrono::steady_clock::now();

    ScanResults results = run_scan(*target, opts.ports, opts.timeout, opts.udp, opts.workers);

    std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

    print_report(opts, ip_to_string(*target), results, elapsed.count());

    return 0;
}
