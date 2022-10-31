#ifndef SPONGE_LIBSPONGE_BYTE_STREAM_HH
#define SPONGE_LIBSPONGE_BYTE_STREAM_HH

#include <string>

//! \brief An in-order byte stream.

//! Bytes are written on the "input" side and read from the "output"
//! side.  The byte stream is finite: the writer can end the input,
//! and then no more bytes can be written.
class ByteStream {
  private:
    // Your code here -- add private members as necessary.
    // 不需要使用复杂的数据结构，需要存储容量的上限
    // 容量的上限就是分配的内存的大小
    // Hint: This doesn't need to be a sophisticated data structure at
    // all, but if any of your tests are taking longer than a second,
    // that's a sign that you probably want to keep exploring
    // different approaches.
    size_t _capacity;
    size_t _bytes_written;
    size_t _bytes_read;
    // 使用一个string模拟字节流，string可以近似为无限长，对应于没有限制的流的长度
    // 但容量有限，即只能读取有限长度的内容
    std::string str;
    bool _end_input;
    bool _error;  //!< Flag indicating that the stream suffered an error.

  public:
    //! Construct a stream with room for `capacity` bytes.
    ByteStream(const size_t capacity);

    //! \name "Input" interface for the writer
    //!@{

    //! Write a string of bytes into the stream. Write as many
    //! as will fit, and return how many were written.
    //! \returns the number of bytes accepted into the stream
    // 尽可能多的写入，直到容量满为止，返回写入的字节数目
    // 没写入的应该在外面等着
    size_t write(const std::string &data);

    //! \returns the number of additional bytes that the stream has space for
    // 返回流中剩余的容量
    size_t remaining_capacity() const;

    //! Signal that the byte stream has reached its ending
    // 输入流是否终止的flag
    void end_input();

    //! Indicate that the stream suffered an error.
    void set_error() { _error = true; }
    //!@}

    //! \name "Output" interface for the reader
    //!@{

    //! Peek at next "len" bytes of the stream
    //! \returns a string
    // 查看流中下面len长的字节，但不会将这些字节移出buffer
    std::string peek_output(const size_t len) const;

    //! Remove bytes from the buffer
    // 将一些字节移出buffer
    void pop_output(const size_t len);

    //! Read (i.e., copy and then pop) the next "len" bytes of the stream
    //! \returns a string
    // 相当于执行了peek+pop
    std::string read(const size_t len);

    //! \returns `true` if the stream input has ended
    // 判断输入是否结束
    bool input_ended() const;

    //! \returns `true` if the stream has suffered an error
    bool error() const { return _error; }

    //! \returns the maximum amount that can currently be read from the stream
    // 返回流中剩余未移出的字节流大小
    size_t buffer_size() const;

    //! \returns `true` if the buffer is empty
    bool buffer_empty() const;

    //! \returns `true` if the output has reached the ending
    bool eof() const;
    //!@}

    //! \name General accounting
    //!@{

    //! Total number of bytes written
    size_t bytes_written() const;

    //! Total number of bytes popped
    size_t bytes_read() const;
    //!@}
};

#endif  // SPONGE_LIBSPONGE_BYTE_STREAM_HH
