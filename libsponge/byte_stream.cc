#include "byte_stream.hh"

#include <algorithm>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _capacity(capacity), buffer(), _end_input(false), _bytes_read(0), _bytes_write(0) {}

size_t ByteStream::write(const string &data) {
    if (this->_capacity == this->buffer.size()) {
        return 0;
    }
    size_t bytes_acc = min(data.size(), this->_capacity - this->buffer.size());
    for (size_t i = 0; i < bytes_acc; i++) {
        this->buffer.push_back(data[i]);
    }
    _bytes_write += bytes_acc;
    return bytes_acc;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string ret;
    int bytes_read = min(len, buffer.size());
    for (int i = 0; i < bytes_read; i++) {
        ret.push_back(buffer[i]);
    }
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    int bytes_pop = min(len, buffer.size());
    for (int i = 0; i < bytes_pop; i++) {
        buffer.pop_front();
    }
    _bytes_read += bytes_pop;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
string ByteStream::read(const size_t len) {
    string ret = peek_output(len);
    pop_output(len);
    return ret;
}

void ByteStream::end_input() { _end_input = true; }

bool ByteStream::input_ended() const { return _end_input; }

size_t ByteStream::buffer_size() const { return buffer.size(); }

bool ByteStream::buffer_empty() const { return buffer.size() == 0; }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _bytes_write; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer.size(); }
