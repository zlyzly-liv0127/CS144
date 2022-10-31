#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity): _capacity(capacity), _bytes_written(0),
    _bytes_read(0), str(""), _end_input(false), _error(false) { }

size_t ByteStream::write(const string &data) {
    // str现在的内容可以认为是写到
    // 都加入到str后面，但不能超过容量
    // 先这样实现，看看这种理解对不对
    // 好像是错了，不是这么理解的，如果一次write的数据过长，剩余部分会被丢弃
    size_t remain_cap = remaining_capacity();
    if (remain_cap == 0)
        // 已经无法写入了，不需要做任何改变
        return 0;
    // str留下的都是能写入的，最后再重构str
    // 保证write完str就是实际的缓存区内容
    // 当下剩余的str还没有被读取，且已经被写入
    size_t len = data.size();
    size_t res;
    if (len >= remain_cap){
        str += data.substr(0, remain_cap);
        res = remain_cap;
    }
    else {
        str += data;
        res = len;
    }
    // _bytes_written返回的是总共写入的字节数
    _bytes_written += res;
    // 这里返回的是这一次写入的字节数，
    return res;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // str不可能超过cap
    if (str.size() >= len)
        return str.substr(0, len);
    else
        return str;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    if (str.size() >= len) {
        str.erase(0, len);
        _bytes_read += len;
    }
    else {
        _bytes_read += str.size();
        str = "";
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string res = peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() { _end_input = true; }

bool ByteStream::input_ended() const { return _end_input; }

size_t ByteStream::buffer_size() const {
    return _bytes_written - _bytes_read;
}

bool ByteStream::buffer_empty() const {
    if (_bytes_read == _bytes_written) return true;
    return false;
}

bool ByteStream::eof() const {
    return buffer_empty() && input_ended();
}

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const {
    return _capacity - _bytes_written + _bytes_read;
}
