#pragma once

#include <string>

#include "parse.h"
#include "threader.h"


void print_report(const Options& opts, const std::string& ip,
                  const ScanResults& results, double seconds);
