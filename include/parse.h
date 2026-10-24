#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


constexpr int MAX_WORKERS = 500;


struct Options {
    std::string host;
    std::vector<int> ports;   // sorted, no duplicates
    int timeout = 1;
    int workers = 100;
    bool udp = false;
    bool color = true;
};


void print_usage(const char* prog);

bool parse_int(std::string_view text, int& out);
bool parse_port(std::string_view text, int& port);
bool parse_range(std::string_view text, std::pair<int, int>& range);
bool parse_ports(std::string_view text, std::vector<int>& ports);

// Returns an exit code if the program should stop, nullopt to keep going
std::optional<int> parse_args(int argc, char* argv[], Options& opts);
