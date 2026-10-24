#pragma once


// Set to "" by init_colors() when colors are off
inline const char* GREEN  = "\033[32m";   // open
inline const char* YELLOW = "\033[33m";   // filtered
inline const char* RED    = "\033[31m";   // closed
inline const char* BOLD   = "\033[1m";
inline const char* DIM    = "\033[2m";
inline const char* RESET  = "\033[0m";


// Off with --no-color, NO_COLOR, or when stdout isn't a terminal
void init_colors(bool enabled);
