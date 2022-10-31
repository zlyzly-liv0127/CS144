#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class Retransmission_Timer {
    // 重传计时器需要实现什么
  private:
    // 需要一个RTO
    unsigned int retransmission_timeout;
    // 重传计时器启动到现在总共经过的毫秒数
    size_t ms_all;
    // 记录重传次数，初始化为0
    unsigned int consecutive_retransmissions;

  public:
    // 构造函数
    Retransmission_Timer(unsigned int retx_timeout);

    // 检查是否超时
    bool check();

    // 将RTO加倍
    void set_doubleRTO();

    // 重置RTO
    void reset_RTO(unsigned int retx_timeout);

    // 重置时间
    void reset_ms();

    // 重置重传次数
    void reset_rets();

    // 增加总耗时
    void add_ms(size_t ms_since_last_tick);

    // 增加重传次数
    void add_rets() { consecutive_retransmissions += 1; }
    // 返回重传次数
    unsigned int get_rets() const { return consecutive_retransmissions; }

    size_t get_ms_all() const { return ms_all;}
};

class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    // 发送端的isn
    WrappingInt32 _isn;
    // 重传计时器
    Retransmission_Timer _ret_timer;
    //! outbound queue of segments that the TCPSender wants sent
    // TCPSegment队列，可能需要拷贝一分以追踪为完成的segment
    std::queue<TCPSegment> _segments_out{};
    // 上面队列的拷贝，存储未被确认的数据报，用deque是为了方便遍历
    std::deque<TCPSegment> _segments_out_2{};

    //! retransmission timer for the connection
    // RTO的初始值
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    // 要发送的字节流
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    // 下一个要发送字节的序列号
    // 这需要考虑重传吗？先不考虑看看试试
    uint64_t _next_seqno{0};

    // 需要维护对端的ackno和窗口大小
    // ackno初始化为isn即可，对应没有发送任何字节
    WrappingInt32 _ackno;
    uint16_t _window_size;

    // 记录是否发过syn和fin
    bool _syn;
    bool _fin;

  public:
    //! Initialize a TCPSender
    // 让三个常量作为构造函数的参数
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    // length_in_sequence_space() 返回一个segment所占的序列号长度，包括了SYN和FIN
    // 目前预想的处理是每个segment完全确认才算完成，这里就返回所有未完成的segment所占序列号的长度之和
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}
    std::deque<TCPSegment> &segments_out_2() { return _segments_out_2; }

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
    size_t get_ms_all() const { return _ret_timer.get_ms_all(); }

    Retransmission_Timer &ret_Timer() { return _ret_timer; }

    WrappingInt32 get_ackno() const { return _ackno; }

    bool get_syn() const { return _syn; }
    bool get_fin() const { return _fin;}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
