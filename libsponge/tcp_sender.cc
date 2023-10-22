#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _send_window(TCPConfig().MAX_PAYLOAD_SIZE) {
    TCPSegment init_segment{};
    // init_segment.payload() = Buffer("");
    init_segment.header().syn = true;
    init_segment.header().seqno = _isn;
    push_segment(init_segment);
}

uint64_t TCPSender::bytes_in_flight() const {
    uint64_t sum = 0;
    for (const TCPSegment &segment : _outstanding_segments) {
        sum += segment.length_in_sequence_space();
    }
    return sum;
}

void TCPSender::push_segment(const TCPSegment &segment) {
    _segments_out.push(segment);
    _outstanding_segments.push_back(segment);
}

void TCPSender::fill_window() {
    // form TCP sengemt
    TCPSegment segment{};
    int segment_len = min(_stream.buffer_size(), _send_window);
    if (segment_len == 0)
        return;
    segment.payload() = Buffer(_stream.read(segment_len));
    segment.header().seqno = next_seqno();
    _segments_out.push(segment);
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _next_seqno = unwrap(ackno, _isn, _next_seqno);
    clear_outstanding_segments(_next_seqno);
    DUMMY_CODE(window_size);
    return {};
}

void TCPSender::clear_outstanding_segments(uint64_t ackno) {}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

unsigned int TCPSender::consecutive_retransmissions() const { return {}; }

void TCPSender::send_empty_segment() {}
