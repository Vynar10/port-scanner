#include "threader.h"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>

#include <unistd.h>

#include "colors.h"
#include "tcp.h"
#include "udp.h"


ScanResults run_scan(const sockaddr_in& target, const std::vector<int>& ports,
                     int timeout_max, bool udp, int workers) {
    ScanResults results;

    const size_t total = ports.size();
    size_t scanned = 0;

    // only show the counter in a terminal
    const bool show_progress = isatty(STDERR_FILENO);

    // each worker takes the next index, so no port is scanned twice
    std::atomic<size_t> next{0};

    // lock only after the slow part
    std::mutex results_mutex;

    auto worker = [&]() {
        while (true) {
            size_t i = next++;

            if (i >= total) {
                return;
            }

            int port = ports[i];

            // own copy per worker
            sockaddr_in addr = target;
            addr.sin_port = htons(static_cast<uint16_t>(port));

            PortResult result = udp ? scan_udp_port(addr, timeout_max)
                                    : scan_tcp_port(addr, timeout_max);

            std::lock_guard<std::mutex> lock(results_mutex);

            switch (result.status) {
                case PortStatus::Open:
                    results.open.push_back({port, result.info});
                    break;

                case PortStatus::Closed:
                    results.closed.push_back(port);
                    break;

                case PortStatus::Filtered:
                    results.filtered.push_back(port);
                    break;

                case PortStatus::Error:
                    results.errors.push_back({port, result.info});
                    break;
            }

            ++scanned;

            if (show_progress) {
                std::cerr << "\r\033[2K  " << DIM << "scanning " << RESET
                          << scanned << "/" << total
                          << DIM << "  ·  " << RESET
                          << GREEN << results.open.size() << " open" << RESET
                          << std::flush;
            }
        }
    };

    workers = static_cast<int>(std::min<size_t>(workers, total));

    std::vector<std::thread> threads;
    threads.reserve(workers);

    for (int i = 0; i < workers; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    if (show_progress) {
        std::cerr << "\r\033[2K" << std::flush;   // wipe the progress line
    }

    // workers finish in random order
    std::sort(results.open.begin(), results.open.end(),
              [](const OpenPort& a, const OpenPort& b) { return a.port < b.port; });
    std::sort(results.closed.begin(), results.closed.end());
    std::sort(results.filtered.begin(), results.filtered.end());
    std::sort(results.errors.begin(), results.errors.end());

    return results;
}
