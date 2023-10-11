#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    // if the segment contains syn
    const TCPHeader header = seg.header();
    if (header.syn) {
        if (!_syn_received) {
            _syn_received = true;
            // init isn and ackno
            _isn = header.seqno;
            _ackno = header.seqno + 1;
            return true;
        } else {
            return false;
        }
    }
    // if the segment contains fin
    return {};
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!this->_syn_received) {
        return {};
    }
    return _ackno;
}

size_t TCPReceiver::window_size() const { return {}; }
