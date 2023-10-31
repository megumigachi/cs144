#ifndef LIBSPONGE_LOGGER
#define LIBSPONGE_LOGGER

#include <iostream>

#define DEBUG 1

void log(std::string output) {
    if (DEBUG) {
        std::cout << output << std::endl;
    }
}

#endif