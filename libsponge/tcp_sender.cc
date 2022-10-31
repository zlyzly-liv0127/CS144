#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// 重传计时器构造函数
Retransmission_Timer::Retransmission_Timer(unsigned int retx_timeout): retransmission_timeout(retx_timeout)
    , ms_all(0)
    , consecutive_retransmissions(0) {}

// 返回true说明超时
bool Retransmission_Timer::check() {
    return ms_all >= retransmission_timeout;
}

void Retransmission_Timer::set_doubleRTO() {
    retransmission_timeout = 2 * retransmission_timeout;
}

void Retransmission_Timer::reset_ms() {
    ms_all = 0;
}

void Retransmission_Timer::reset_rets() {
    consecutive_retransmissions = 0;
}

void Retransmission_Timer::reset_RTO(unsigned int retx_timeout) {
    retransmission_timeout = retx_timeout;
}

void Retransmission_Timer::add_ms(size_t ms_since_last_tick) {
    ms_all += ms_since_last_tick;
}

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
// 初始化了isn，要发送的字节流的容量以及RTO的初始值
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _ret_timer(retx_timeout)
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _ackno(_isn)
    , _window_size(1)
    , _syn(false)
    , _fin(false){}

// length_in_sequence_space() 返回一个segment所占的序列号长度，包括了SYN和FIN
// 目前预想的处理是每个segment完全确认才算完成，这里就返回所有未完成的segment所占序列号的长度之和
// 发送的内容使我们自己从输入的字节流中选的，不会发重的内容(没意义)
uint64_t TCPSender::bytes_in_flight() const {
    uint64_t res = 0;
    for (auto &seg: _segments_out_2) {
        res += seg.length_in_sequence_space();
    }
    return res;
}

void TCPSender::fill_window() {
    // 核心函数，实现segment的发送
    // 如何构造TCPsegment？没给构造函数，尝试用parse构造
    // 应该单独出来一个函数进行构造
    // bfsz是可以读取的最大字节长度
    // 这里不考虑重传，只考虑发送新的数据报
    // 接收窗口的区间是[ackno, achno + window_size) 都是序列号
    // 发送的起点是next_seqno()的返回值，也是序列号
    // 这里应该写成一个循环，只要能发就一直发
    while (true) {
        size_t bfsz = _stream.buffer_size();
        if (bfsz == 0 && _syn && (_fin || !_stream.input_ended()))
            // 流没有数据且syn已经发送且不能发送fin直接返回
            break;
        uint16_t _tmp;
        if (_window_size == 0)
            _tmp = 1;
        else
            _tmp = _window_size;
        // 这个函数不会修改_window_size,后面的_window_size全部用_tmp替代
        if (_ackno + _tmp - next_seqno() == 0)
            // 没有空间，直接返回，先这样处理试试看
            break;
        // 可以发送相关帧
        string _payload;
        TCPSegment seg;
        // uint16_t _maxsize;
        // uint16_t _tmp;
        // 构造seg，payload的长度有window_size和读取的长度共同决定
        // _windows_size至少为1
        if (!_syn) {
            // 要发syn
            uint16_t _maxsize = _ackno + _tmp - next_seqno() - 1;
            if (bfsz <= _maxsize) {
                if (bfsz <= TCPConfig::MAX_PAYLOAD_SIZE) {
                    // 不超过最大payload上限，全部读取
                    _payload = _stream.read(bfsz);
                }
                else {
                    // 超过，读取上限
                    _payload = _stream.read(TCPConfig::MAX_PAYLOAD_SIZE);
                }
            }
            else {
                if (_maxsize <= TCPConfig::MAX_PAYLOAD_SIZE) {
                    // 不超过最大payload上限，全部读取
                    _payload = _stream.read(_maxsize);
                }
                else {
                    // 超过，读取上限
                    _payload = _stream.read(TCPConfig::MAX_PAYLOAD_SIZE);
                }
            }
            // 不再修改window_size，只在收到ack时修改
            // _window_size -= _payload.size() + 1;
            _next_seqno = _payload.size() + 1;
            seg.payload() = Buffer(move(_payload));
            // seg.parse(Buffer(move(_payload)));
            seg.header().syn = true;
            seg.header().seqno = _isn;
            _syn = true;
            // _next_seqno = _payload.size() + 1;
        }
        else {
            // 不发syn
            // 预先读出bytes_read()，这些字节已经发送了
            size_t _bytes_read = _stream.bytes_read();
            // 如果ackno为2 sz为5 next_seq为4
            // 接收窗口为[2, 7) 实际会发[4,7)
            uint16_t _maxsize = _ackno + _tmp - next_seqno();
            if (bfsz <= _maxsize) {
                if (bfsz <= TCPConfig::MAX_PAYLOAD_SIZE) {
                    // 不超过最大payload上限，全部读取
                    _payload = _stream.read(bfsz);
                }
                else {
                    // 超过，读取上限
                    _payload = _stream.read(TCPConfig::MAX_PAYLOAD_SIZE);
                }
            }
            else {
                if (_maxsize <= TCPConfig::MAX_PAYLOAD_SIZE) {
                    // 不超过最大payload上限，全部读取
                    _payload = _stream.read(_maxsize);
                }
                else {
                    // 超过，读取上限
                    _payload = _stream.read(TCPConfig::MAX_PAYLOAD_SIZE);
                }
            }
            // _window_size -= _payload.size();
            _next_seqno = _next_seqno + _payload.size();
            seg.payload() = Buffer(move(_payload));
            // seg.parse(Buffer(move(_payload)));
            // 这里的序列号不对的，需要重新设置
            // 序列号要跟全部读取的字节数相关
            // 这里不考虑重发的问题,从流中读取的内容都视为已经发送了
            seg.header().seqno = _isn + _bytes_read + 1;
            // _next_seqno = _next_seqno + _payload.size();
        }
        // 下面这里由input_ended改为了eof，试一下
        // input_ended只是流没有后续的输入，但之前存在buffer里的可能还没有完全read，
        // 可能会出现发了fin帧但实际上出站流还没完全发完的情况
        if (!_fin && _stream.eof()) {
            // 流输入已经结束，可以发送fin
            // cerr << "can send fin!! " << endl;
            // 走到这一步但是fin却没发出去
            // 这一轮发不出去能不能下一轮发
            if (_ackno + _tmp - next_seqno() > 0) {
                // 有空间才能发送fin
                // 此时next_seqno已经修改过
                _next_seqno += 1;
                seg.header().fin = true;
                _fin = true;
                // cerr << "send fin to peer!!" << endl;
            }
            // seg.header().fin = true;
        }
        // 第一次发要带syn，什么时候带fin，当流的input结束时
        // syn帧和fin帧都只发一次，需要记录
        // 发送数据报
        _segments_out.push(seg);
        _segments_out_2.push_back(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // 收到的值不是合理的ack，直接返回
    if (ackno - next_seqno() > 0)
        // ack比还没发的还大，直接返回
        return;
    // 需要重置重传计时器
    // 思考一下这个逻辑，发了1 2 两个帧，此时收到1帧的ack，重置计时器，等待2帧，没毛病
    // 发了1 2 两个帧， 1帧重传两次， 终于收到1帧，重置，之后会定时重传2帧，也没问题
    bool flag = false;
    // 更新两个值
    // 这个值完全没用上，要结合已经发的值更新

    // 遍历所有未完成的seg
    while (!_segments_out_2.empty()) {
        // 经此处理后，所有终点在ackno之前的段都pop掉了
        auto &seg = _segments_out_2.front();
        WrappingInt32 end_no = seg.header().seqno + seg.payload().size() - 1;
        if (seg.header().syn)
            end_no = end_no + 1;
        if (seg.header().fin)
            end_no = end_no + 1;
        if (ackno - end_no > 0) {
            // 这样的判断是有风险的，如果ackno比endno大得多，可能会小于0，暂时不考虑
            _segments_out_2.pop_front();
            flag = true;
        }
        else
            break;
    }
    if (flag) {
        // RTO改为初始值
        _ret_timer.reset_RTO(_initial_retransmission_timeout);
        // 重置重传次数
        _ret_timer.reset_rets();
        if (!_segments_out_2.empty()) {
            // 还有未完成数据,重置计时器
            _ret_timer.reset_ms();
        }
    }
    // 开启了新空间，再次调用
    // 接收窗口的区间是[ackno, achno + window_size)
    // ackno之前的都已经接受了，如何更新可发的区间
    _ackno = ackno;
    _window_size = window_size;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
// 输入的参数是该函数和上次调用时的时间间隔，考虑统计总时间，判断是否需要重传？
// 需要实现重传计时器

void TCPSender::tick(const size_t ms_since_last_tick) {
    // 这个函数就是负责重传的

    // 增加总时长
    _ret_timer.add_ms(ms_since_last_tick);
    // 判断是否超时
    if (_ret_timer.check()) {
        // 1.重传最早帧
        // std::cerr << "the size of q: " << segments_out().size() << endl;
        // std::cerr << "the size of dq: " << segments_out_2().size() << endl;
        if (_segments_out_2.empty()){
            // 所有帧都已经ack，重置并返回
            _ret_timer.reset_ms();
            _ret_timer.reset_rets();
            _ret_timer.reset_RTO(_initial_retransmission_timeout);
            return;
        }
        auto &seg = _segments_out_2.front();
        _segments_out.push(seg);
        // 2.如果窗口大小非0，增加重传次数并double RTO
        // 需要维护窗口大小吗？
        // 为什么这个要和窗口大小有关？？？？？
        // 如果窗口为0，重传不会有效果？
        if (_window_size > 0) {
            _ret_timer.add_rets();
            _ret_timer.set_doubleRTO();
        }
        // 3.重置重传计数器并启动，此时的RTO已经double过
        _ret_timer.reset_ms();
    }
}

// 返回重传计时器记录的重传次数
unsigned int TCPSender::consecutive_retransmissions() const {
    return _ret_timer.get_rets();
}

void TCPSender::send_empty_segment() {
    // 无SYN、FIN以及payload，只需要设置合适的序列号
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
    // 不需要跟踪这样的数据报
    //_segments_out_2.push_back(seg);
}
