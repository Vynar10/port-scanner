#include "colors.h"

#include <cstdlib>

#include <unistd.h>


void init_colors(bool enabled) {
    if (enabled && std::getenv("NO_COLOR") == nullptr && isatty(STDOUT_FILENO)) {
        return;
    }

    GREEN = YELLOW = RED = BOLD = DIM = RESET = "";
}
