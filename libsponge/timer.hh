#ifndef SPONGE_LIBSPONGE_TIMER_HH
#define SPONGE_LIBSPONGE_TIMER_HH

struct Timer {
    unsigned int _remaining_ms{0};
    bool _start{false};

    void start(unsigned int ms) {
        _start = true;
        _remaining_ms = ms;
    }

    void time_passed(unsigned int ms) { _remaining_ms -= ms; }

    bool expired() { return _start && _remaining_ms <= 0; }

    void turn_off() { _start = false; }
};

#endif