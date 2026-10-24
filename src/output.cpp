#include "output.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <utility>
#include <vector>

#include <netdb.h>
#include <netinet/in.h>

#include "colors.h"


constexpr size_t MAX_RANGES_SHOWN = 8;

constexpr size_t MAX_ERRORS_SHOWN = 5;


// pad before adding colors, the escape codes mess up the width
static std::string pad(const std::string& text, size_t width) {
    return text.size() >= width ? text + " " : text + std::string(width - text.size(), ' ');
}


// 22 -> ssh, from /etc/services
static std::string service_name(int port, bool udp) {
    servent* service = getservbyport(htons(static_cast<uint16_t>(port)), udp ? "udp" : "tcp");
    return service ? service->s_name : "unknown";
}


// {1, 2, 3, 7, 9, 10} -> {1-3, 7, 9-10}
static std::vector<std::pair<int, int>> to_ranges(const std::vector<int>& ports) {
    std::vector<std::pair<int, int>> ranges;

    for (int port : ports) {
        if (!ranges.empty() && ranges.back().second + 1 == port) {
            ranges.back().second = port;
        } else {
            ranges.push_back({port, port});
        }
    }

    return ranges;
}


// "1-21, 23-79, 81-1000"
static std::string describe_ranges(const std::vector<int>& ports, size_t max_shown) {
    std::vector<std::pair<int, int>> ranges = to_ranges(ports);
    std::string text;

    for (size_t i = 0; i < ranges.size() && i < max_shown; ++i) {
        auto [first, last] = ranges[i];

        if (i > 0) {
            text += ", ";
        }

        text += std::to_string(first);

        if (last != first) {
            text += "-" + std::to_string(last);
        }
    }

    if (ranges.size() > max_shown) {
        text += "  +" + std::to_string(ranges.size() - max_shown) + " more";
    }

    return text;
}


static std::string format_seconds(double seconds) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.2fs", seconds);
    return text;
}


static void print_group(const char* color, const std::string& label, size_t count, bool last) {
    std::cout << "  │\n"
              << "  " << (last ? "└── " : "├── ")
              << color << BOLD << pad(label, 16) << RESET
              << color << count << RESET << "\n";
}


static void print_leaf(bool group_last, bool leaf_last, const std::string& text) {
    std::cout << "  " << (group_last ? "    " : "│   ")
              << (leaf_last ? "└── " : "├── ")
              << text << "\n";
}


static void print_range_group(const char* color, const std::string& label,
                              const std::vector<int>& ports, bool last) {
    print_group(color, label, ports.size(), last);

    if (!ports.empty()) {
        print_leaf(last, true, std::string(color) + describe_ranges(ports, MAX_RANGES_SHOWN) + RESET);
    }
}


void print_report(const Options& opts, const std::string& ip,
                  const ScanResults& results, double seconds) {
    const std::string proto = opts.udp ? "udp" : "tcp";
    const bool has_errors = !results.errors.empty();

    std::cout << "\n  " << BOLD << opts.host << RESET;

    if (opts.host != ip) {
        std::cout << "  " << DIM << "(" << ip << ")" << RESET;
    }

    const size_t workers = std::min<size_t>(opts.workers, opts.ports.size());

    std::cout << "\n  " << DIM
              << (opts.udp ? "UDP" : "TCP") << " scan"
              << "  ·  ports " << describe_ranges(opts.ports, 4)
              << "  ·  " << workers << (workers == 1 ? " worker" : " workers")
              << "  ·  " << format_seconds(seconds)
              << RESET << "\n";

    print_group(GREEN, "open", results.open.size(), false);

    for (size_t i = 0; i < results.open.size(); ++i) {
        const OpenPort& p = results.open[i];

        print_leaf(false, i + 1 == results.open.size(),
                   std::string(GREEN) + pad(std::to_string(p.port) + "/" + proto, 12) + RESET
                   + DIM + pad(service_name(p.port, opts.udp), 16) + RESET
                   + p.banner);
    }

    // UDP can't tell open from filtered when nothing answers
    print_range_group(YELLOW, opts.udp ? "open|filtered" : "filtered", results.filtered, false);
    print_range_group(RED, "closed", results.closed, !has_errors);

    if (has_errors) {
        print_group(RED, "errors", results.errors.size(), true);

        size_t shown = std::min(results.errors.size(), MAX_ERRORS_SHOWN);

        for (size_t i = 0; i < shown; ++i) {
            const auto& [port, message] = results.errors[i];
            bool leaf_last = i + 1 == shown && results.errors.size() <= MAX_ERRORS_SHOWN;

            print_leaf(true, leaf_last, pad(std::to_string(port) + "/" + proto, 12) + message);
        }

        if (results.errors.size() > MAX_ERRORS_SHOWN) {
            print_leaf(true, true, std::string(DIM) + "+"
                       + std::to_string(results.errors.size() - MAX_ERRORS_SHOWN) + " more" + RESET);
        }
    }

    std::cout << std::endl;
}
