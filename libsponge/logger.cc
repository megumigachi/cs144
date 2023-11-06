// logger.cpp
#include "logger.hh"

void log(const std::string &output) {
    if (DEBUG) {
        std::cout << output << std::endl;
    }
}
