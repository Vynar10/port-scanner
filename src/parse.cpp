#include "parse.h"

#include <algorithm>
#include <charconv>
#include <iostream>


static std::vector<int> port_span(int first, int last) {
    std::vector<int> ports;
    ports.reserve(last - first + 1);

    for (int p = first; p <= last; ++p) {
        ports.push_back(p);
    }

    return ports;
}


void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " <host> [options]\n"
              << "\n"
              << "Options:\n"
              << "  -p <ports>    ports to scan: 80, 1-1024, 22,80,443 or a mix (default: 1-1024)\n"
              << "  -p-           scan all ports (1-65535)\n"
              << "  -t <seconds>  timeout per port (default: 1)\n"
              << "  -w <workers>  ports scanned at the same time, 1-" << MAX_WORKERS << " (default: 100)\n"
              << "  -sU           UDP scan instead of TCP\n"
              << "  --no-color    plain output\n"
              << "  -h            show this help\n"
              << "\n"
              << "Examples:\n"
              << "  " << prog << " scanme.nmap.org -p 1-100 -t 2\n"
              << "  " << prog << " scanme.nmap.org -p 22,80,443\n"
              << "  " << prog << " 127.0.0.1 -p- -w 200\n"
              << "  " << prog << " 127.0.0.53 -p 53 -sU\n";
}


// "80" works, "80abc" and " 80" don't
bool parse_int(std::string_view text, int& out) {
    const char* end = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(text.data(), end, out);

    return ec == std::errc() && ptr == end;
}


bool parse_port(std::string_view text, int& port) {
    return parse_int(text, port) && port >= 1 && port <= 65535;
}


// "80" -> {80, 80}, "1-1024" -> {1, 1024}
bool parse_range(std::string_view text, std::pair<int, int>& range) {
    size_t dash = text.find('-');

    if (dash == std::string_view::npos) {
        if (!parse_port(text, range.first)) {
            return false;
        }

        range.second = range.first;
        return true;
    }

    return parse_port(text.substr(0, dash), range.first)
        && parse_port(text.substr(dash + 1), range.second)
        && range.first <= range.second;
}


// "22", "1-1024", "22,80,8000-8100"
bool parse_ports(std::string_view text, std::vector<int>& ports) {
    std::vector<int> result;

    while (true) {
        size_t comma = text.find(',');
        std::pair<int, int> range;

        if (!parse_range(text.substr(0, comma), range)) {
            return false;
        }

        std::vector<int> span = port_span(range.first, range.second);
        result.insert(result.end(), span.begin(), span.end());

        if (comma == std::string_view::npos) {
            break;
        }

        text.remove_prefix(comma + 1);
    }

    // remove duplicates
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    ports = std::move(result);
    return true;
}


std::optional<int> parse_args(int argc, char* argv[], Options& opts) {
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        auto value = [&]() -> std::optional<std::string_view> {
            if (i + 1 >= argc) {
                return std::nullopt;
            }

            return argv[++i];
        };

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;

        } else if (arg == "-p-") {
            opts.ports = port_span(1, 65535);

        } else if (arg == "-sU") {
            opts.udp = true;

        } else if (arg == "--no-color") {
            opts.color = false;

        } else if (arg == "-p") {
            auto v = value();

            if (!v || !parse_ports(*v, opts.ports)) {
                std::cerr << "-p needs ports like 80, 1-1024 or 22,80,443 (each 1-65535).\n";
                return 1;
            }

        } else if (arg == "-t") {
            auto v = value();

            if (!v || !parse_int(*v, opts.timeout) || opts.timeout < 1) {
                std::cerr << "-t needs a whole number of seconds (1 or more).\n";
                return 1;
            }

        } else if (arg == "-w") {
            auto v = value();

            if (!v || !parse_int(*v, opts.workers) || opts.workers < 1 || opts.workers > MAX_WORKERS) {
                std::cerr << "-w needs a number of workers from 1 to " << MAX_WORKERS << ".\n";
                return 1;
            }

        } else if (arg.starts_with('-')) {
            std::cerr << "Unknown option: " << arg << "\n\n";
            print_usage(argv[0]);
            return 1;

        } else if (opts.host.empty()) {
            opts.host = arg;

        } else {
            std::cerr << "Only one host can be scanned at a time.\n";
            return 1;
        }
    }

    if (opts.host.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    if (opts.ports.empty()) {
        opts.ports = port_span(1, 1024);
    }

    return std::nullopt;
}
