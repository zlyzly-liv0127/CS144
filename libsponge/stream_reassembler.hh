#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>
#include <deque>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    // 要存储每次输入的string，使用什么样的数据结构
    // 需要记录当前已经装配到哪里了，读到哪里了
    // 流总长度未知，必须使用动态数据结构
    // deque
    // 已经读取的内容还需不需要保存？不需要，我们需要维护一个动态数组，以便可以再两端迅速增删元素，同时数据的大小不能超过cap
    // 重新组装的结果放在哪里？应该是要和_output结合吧
    // 调用write操作？感觉是
    // 不能用空格标记空字符，考虑再用一个deque进行同步
    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
    uint64_t start; // dq中第一个元素的index，对应图中绿色第一个
    uint64_t unassembled; //dq中第一个未装配的index，对应图中绿色后面的第一个
    bool eof1;
    uint64_t last_idx;
    std::deque<char> dq;
    std::deque<int> dq2;

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    // 构造一个ByteStream对象，返回已经装配好的字符串，似乎不需要自己实现，有点不太理解
    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    // 存储的其实就是红色部分的字节数
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    // 没有红字，返回true
    bool empty() const;


    // 检查是否有read调用，并修改dq和start
    void check();
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
