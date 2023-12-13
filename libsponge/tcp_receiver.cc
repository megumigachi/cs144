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
            _isn = header.seqno;
            _ackno = header.seqno + 1;
            if (header.fin) {
                _reassembler.push_substring("", convert_i32_to_absolute(_ackno) - 1, true);
                _ackno = _ackno + 1;
            }
            return true;
        } else {
            return false;
        }
    }

    bool fin_recv = header.fin;
    WrappingInt32 seq_no = header.seqno;

    // examine if the segment overflow the receive window
    string_view data = seg.payload().str();
    int64_t max_data_length = convert_i32_to_absolute(_ackno) + window_size() - convert_i32_to_absolute(seq_no);
    if (max_data_length < 0) {
        return false;
    }

    if (max_data_length == 0 && !fin_recv) {
        return false;
    }

    // if the right part overflow,  trim the string
    if (max_data_length < static_cast<int64_t>(data.size())) {
        data = data.substr(0, max_data_length);
    }

    // if the left part overflow
    if (convert_i32_to_absolute(seq_no) < convert_i32_to_absolute(_ackno)) {
        // if there is no overlapping area between recv window and received data, reject it

        // test fsm_listen_relaxed: fix a bug: convert_i32_to_absolute(seq_no) + data.size() - 1
        // may less than 0 and lead to a overflow(cuz it is an unsigned int)
        if (convert_i32_to_absolute(seq_no) + data.size() < convert_i32_to_absolute(_ackno) + 1) {
            return false;
        } else {
            // trim the left part or leave it to reassembler?
            size_t start_idx = convert_i32_to_absolute(_ackno) - convert_i32_to_absolute(seq_no);
            data = data.substr(start_idx);
            seq_no = _ackno;
        }
    }

    uint64_t stream_index = convert_i32_to_absolute(seq_no) - 1;
    _reassembler.push_substring(string(data), stream_index, fin_recv);
    _ackno = convert_absolute_to_i32(_reassembler.stream_index() + 1) + fin_recv;
    // update checkpoint
    _checking_point = stream_index;
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!this->_syn_received) {
        return {};
    }
    return _ackno;
}

size_t TCPReceiver::window_size() const { return _capacity - stream_out().buffer_size(); }
