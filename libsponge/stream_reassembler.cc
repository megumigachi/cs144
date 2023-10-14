#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _cur_index(0)
    , _unassembled_strs()
    , _unassembled_bytes(0)
    , _eof_sign(false) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    bool _eof = eof;
    // remove duplicates
    string _data = data;
    if (_cur_index > index) {
        if (data.size() + index < _cur_index) {
            return;
        } else {
            int begin_idx = _cur_index - index;
            _data = data.substr(begin_idx);
            return push_substring(_data, _cur_index, eof);
        }
    }
    // remove all unassembled substrings that will be overlapped.
    // Step 1: Remove fully covered strings
    if (!_unassembled_strs.empty()) {
        for (auto it = _unassembled_strs.begin(); it != _unassembled_strs.end();) {
            size_t start_idx = it->first;
            size_t end_idx = start_idx + it->second.length() - 1;
            if (index <= start_idx && index + data.length() - 1 >= end_idx) {
                // Current string is fully covered by the new string
                _unassembled_bytes -= it->second.length();
                it = _unassembled_strs.erase(it);  // erase returns iterator to the next element
            } else {
                ++it;
            }
        }
    }

    // Step 2: Modify data to ensure no overlap
    std::string modified_data = data;
    if (!_unassembled_strs.empty()) {
        for (const auto &pair : _unassembled_strs) {
            size_t start_idx = pair.first;
            size_t end_idx = start_idx + pair.second.length() - 1;
            if (index < start_idx && index + modified_data.length() - 1 >= start_idx) {
                // Trim the end of modified_data to prevent overlap
                modified_data = modified_data.substr(0, start_idx - index);
            } else if (index <= end_idx && index + modified_data.length() - 1 > end_idx) {
                // Trim the end of the existing string in _unassembled_strs to prevent overlap
                _unassembled_strs[pair.first] = pair.second.substr(0, index - pair.first);
            } else if (start_idx <= index && end_idx >= index + modified_data.length() - 1) {
                // if data is fully covered, return
                return;
            } else if (start_idx > index + modified_data.length()) {
                break;
            }
        }
    }
    // deal with overflow situation
    size_t assembled_bytes = _output.buffer_size();
    if (assembled_bytes + modified_data.size() > _capacity) {
        int end_index = _output.remaining_capacity();
        if (end_index == 0)
            return;
        modified_data = modified_data.substr(0, end_index);
        _eof = false;
    }
    if (_eof) {
        _eof_sign = true;
    }
    _unassembled_bytes += modified_data.length();
    _unassembled_strs[index] = move(modified_data);
    // when current index==index, write data to bytestream.
    if (_cur_index == index) {
        write_data();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }

void StreamReassembler::write_data() {
    while (_unassembled_strs.find(_cur_index) != _unassembled_strs.end()) {
        string to_add = _unassembled_strs[_cur_index];
        _output.write(to_add);
        _unassembled_strs.erase(_cur_index);
        _unassembled_bytes -= to_add.length();
        _cur_index += to_add.length();
    }
    if (_eof_sign && _unassembled_strs.empty()) {
        _output.end_input();
    }
}
