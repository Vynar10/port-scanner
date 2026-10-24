#pragma once

#include <string>


constexpr int BUFFER_SIZE = 1024;


enum class PortStatus { Open, Closed, Filtered, Error };


struct PortResult {
    PortStatus status;
    std::string info;   // banner when open, error message on error
};
