#include "tcp_connection.hh"

#include "logger.hh"

#include <iostream>
// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _ms_passed - _ms_passed_on_last_segment; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _ms_passed_on_last_segment = _ms_passed;
    if (seg.header().ack) {
        if (!_sender.ack_received(seg.header().ackno, seg.header().win)) {
            log("sender.ack_received returned false");
            //_sender.send_empty_segment();
        }
        send_segments();
    }
    _receiver.segment_received(seg);
}

bool TCPConnection::active() const {
    return (!_sender.stream_in().input_ended()) || (!_receiver.stream_out().input_ended()) ||
           _linger_after_streams_finish;
}

size_t TCPConnection::write(const string &data) {
    DUMMY_CODE(data);
    return {};
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _ms_passed += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
}

void TCPConnection::end_input_stream() { _sender.stream_in().end_input(); }

void TCPConnection::send_segments() {
    _sender.fill_window();
    auto &segments_out = _sender.segments_out();
    while (!segments_out.empty()) {
        _segments_out.push(segments_out.front());
        segments_out.pop();
    }
}

void TCPConnection::connect() { send_segments(); }

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
