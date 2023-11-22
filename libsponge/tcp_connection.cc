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
    // string recv_state = TCPState::state_summary(_receiver);
    // If receiver has not received syn, discard segments without syn
    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::LISTEN && !seg.header().syn) {
        return;
    }
    // pass the segment to receiver and update ackno
    _receiver.segment_received(seg);

    // the second handshake:syn-ack
    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED && (seg.header().syn)) {
        connect();
        return;
    }

    bool need_ack = seg.length_in_sequence_space();

    if (seg.header().ack) {
        if (!_sender.ack_received(seg.header().ackno, seg.header().win)) {
            log("sender.ack_received returned false");
        }
    }
    if (need_ack) {
        _sender.fill_window();
        add_ack_and_send_segments();
    }
}

bool TCPConnection::active() const {
    return (!_sender.stream_in().input_ended()) || (!_receiver.stream_out().input_ended()) ||
           _linger_after_streams_finish;
}

size_t TCPConnection::write(const string &data) {
    size_t written_len = _sender.stream_in().write(data);
    _sender.fill_window();
    return written_len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _ms_passed += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
}

void TCPConnection::end_input_stream() { _sender.stream_in().end_input(); }

void TCPConnection::add_ack_and_send_segments() {
    if (_sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }
    auto &segments_out = _sender.segments_out();
    while (!segments_out.empty()) {
        // put ack number
        TCPSegment seg = segments_out.front();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = 1;
            seg.header().ackno = _receiver.ackno().value();
        }
        _segments_out.push(seg);
        segments_out.pop();
    }
}

void TCPConnection::connect() {
    if (TCPState::state_summary(_sender) != TCPSenderStateSummary::CLOSED) {
        log("cannot connect");
        return;
    }
    _sender.fill_window();
    add_ack_and_send_segments();
}

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
