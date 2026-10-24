#pragma once

#include <netinet/in.h>

#include "common.h"


PortResult scan_tcp_port(const sockaddr_in& target, int timeout_max);
