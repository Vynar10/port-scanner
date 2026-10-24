#pragma once

#include <netinet/in.h>

#include "common.h"


// Filtered means open|filtered here
PortResult scan_udp_port(const sockaddr_in& target, int timeout_max);
