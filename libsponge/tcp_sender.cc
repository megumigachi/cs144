#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <iostream>
#include <random>
#include <stdarg.h>
#include <stdio.h>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

#define DEBUG true

void log(string output) {
    if (DEBUG) {
        cout << output << endl;
    }
}

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _send_window(1)
    , _RTO(retx_timeout) {
    log("-----------------------------------------");
    log("TCP Sender: init");
}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::push_segment(const TCPSegment &segment) {
    log("push segment , content :" + string(segment.payload().str()));
    _segments_out.push(segment);
    _next_seqno += segment.length_in_sequence_space();

    _outstanding_segments.push_back(segment);
    _bytes_in_flight += segment.length_in_sequence_space();

    _timer.start(_RTO);
}

void TCPSender::fill_window() {
    TCPSegment segment{};

    // if syn hasn't been sent , send a syn segment
    if (!_syn_sent) {
        segment.header().syn = true;
        segment.header().seqno = _isn;
        log("sent init segment with syn");
        push_segment(segment);
        _syn_sent = true;
        return;
    }

    // if fin has been sent, the sender will only retransmit segments
    if (_fin_sent) {
        return;
    }

    // form TCP sengemt
    size_t remaining_window = _send_window == 0 ? 1 : _send_window + _ack_recv - _next_seqno;
    size_t segment_len = min(_stream.buffer_size(), remaining_window);

    // if there is nothing to read from buffer,
    // and the input stream didn't reach eof
    // return false
    if (segment_len == 0 && !_stream.eof()) {
        return;
    }

    segment.payload() = Buffer(_stream.read(segment_len));
    segment.header().seqno = next_seqno();

    // if the input buffer reaches eof, and there is remaining window for fin flag
    // send a fin segment
    if (_stream.eof() && remaining_window > segment_len) {
        log("form fin segment");
        segment.header().fin = true;
        _fin_sent = true;
    }
    log("fill window");
    push_segment(segment);
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    log("ack received, abs ack:" + to_string(unwrap(ackno, _isn, _ack_recv)));
    _send_window = window_size;
    uint64_t this_ack = unwrap(ackno, _isn, _ack_recv);
    // if this ack is out of order, do fast retransmitting
    if (this_ack > _next_seqno) {
        return false;
    }
    if (this_ack <= _ack_recv) {
        return true;
    }
    // if new segment is acknowledged, reset retransmit timer
    if (this_ack > _ack_recv) {
        _RTO = _initial_retransmission_timeout;
        _consecutive_retransmissions = 0;
    }

    _ack_recv = this_ack;
    clear_outstanding_segments(_ack_recv);

    // log("ack received,now begin fill window");
    // fill_window();
    return true;
}

void TCPSender::clear_outstanding_segments(uint64_t ackno) {
    while (!_outstanding_segments.empty()) {
        TCPSegment seg = _outstanding_segments.front();
        uint64_t seg_end = unwrap(seg.header().seqno, _isn, _ack_recv) + seg.length_in_sequence_space();
        if (seg_end <= ackno) {
            _outstanding_segments.pop_front();
            _bytes_in_flight -= seg.length_in_sequence_space();
        } else {
            break;
        }
    }
    if (_outstanding_segments.empty()) {
        _timer.turn_off();
    }
}

void TCPSender::retransmit() {
    if (_outstanding_segments.empty()) {
        return;
    }

    _segments_out.push(_outstanding_segments.front());
    // what's the difference between the value of send window is 0 and 1?
    // Idk , but follow the guide of lab3
    // maybe I have a wrong understanding of 'window size'?
    if (_send_window) {
        _consecutive_retransmissions++;
        _RTO = _RTO * 2;
    }
    _timer.start(_RTO);
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer.time_passed(ms_since_last_tick);
    if (_timer.expired()) {
        retransmit();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    log("send empty segment");
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}
