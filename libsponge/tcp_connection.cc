#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const {
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    // 这里应该指的是入站流
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    // 先试一下直接用sender的计时器行不行，感觉不行，要用到其他的变量
    // 收到seg时重置为0
    return _time_since_last_segment;
}

bool TCPConnection::sender_in_fin_acked() const {
    if (_sender.stream_in().eof() && _sender.get_ackno() == _sender.next_seqno() && !_sender.bytes_in_flight()) return true;
    return false;
}
void TCPConnection::segment_received(const TCPSegment &seg) {
    // 重置_time_since_last_segment
    // cout << "segment_received!" << endl;
    // cout << "size of q is: "<< _sender.segments_out().size() << endl;
    if ( !active() )
        return;
    // 收到对fin的ack
    _time_since_last_segment = 0;
    // 如果设置了rst
    if (seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        // 什么叫永久的终止连接？应该就是active返回false
        // active的初始值是false吗？
        // 是不是不该添加下面这一行
        // _linger_after_streams_finish = false;
        _rst  = true;
        return;
    }
    // 将segment提供给TCPReceiver，以便它可以检查传入段上它关心的字段：seqno、syn、payload和fin
    _receiver.segment_received(seg);
    // 如果设置了ack标志，则告诉TCPSender它在传入段上关心的字段：ackno和窗口大小
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    // 判断是否是被动关闭
    // _receiver这一端要不要也改成eof
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        // 入站流结束，出站流还未达到eof
        _linger_after_streams_finish = false;
    }
    // 如果传入的segment占用了任何序列号，TCPConnection确保至少发送一个segment作为应答，以反映ackno和窗口大小中的更新
    // 您的TCPConnection应该回复这些“保持活动”，即使它们不占用任何序列号，
    // 这里和上面可以整合，只要ack有值，就发一个空帧作为ack
    // 不能无脑发ack，如果收到对fin的ack的，且已经ack了对端的fin，直接返回
    // 接收端处在fin_recv且发送端处在FIN_ACKD，直接返回
    // if (_receiver.stream_out().input_ended() && _sender.get_fin() && _sender.get_ackno() == _sender.next_seqno()) {
       // return;
    // }
    // 对于第二个test，这里不能发是因为这里收到的是对fin的ack，我们也受到了对端的fin，且已经对对端的fin进行了ack，当然不需要再发
    // 第一个test，收到的是对端的fin+对己方的ack，还没有对对端的fin进行ack，当然需要ack
    // 收到单纯对于fin的ack也不需要ack，得想清楚啥时候要ack
    // 我先试试实验指导书的写法
    /*
    if (_sender.get_fin() && _fin_ack) {
        // 已经发了fin且ack了对方的fin，直接返回
        return;
    }
    // 判定这一轮是否收到了fin
    if (seg.header().fin)
        _fin_ack = true;
    */
    if (seg.length_in_sequence_space() > 0) {
        // 对端发来的send_empty_segment
        // 先尝试用fill_window发，如果发出去了，就不用再发空帧了
        _sender.fill_window();
        std::queue<TCPSegment> q = std::move(_sender.segments_out());
        size_t sz = q.size();
        if (sz > 0) {
            std::deque<TCPSegment> dq = std::move(_sender.segments_out_2());
            while (sz > 0) {
                dq.pop_back();
                sz--;
            }
            while (!dq.empty()) {
                TCPSegment dq_tmp = dq.front();
                dq.pop_front();
                _sender.segments_out_2().push_back(dq_tmp);
            }
            while (!q.empty()) {
                TCPSegment tmp = q.front();
                q.pop();
                if (_receiver.ackno().has_value()) {
                    tmp.header().ack = true;
                    tmp.header().ackno = _receiver.ackno().value();
                    tmp.header().win = _receiver.window_size();
                }
                _segments_out.push(tmp);
                _sender.segments_out_2().push_back(tmp);
            }
        }
        else {
            _sender.send_empty_segment();
            std::queue<TCPSegment> q1 = std::move(_sender.segments_out());
            // cout << "size of q is: "<< q.size() << endl;
            // 是不是这样处理并未实质的修改TCPSegment
            // 调用send_empty_segment()只会发送一个帧，调用fill_window可能发送多个帧，但这些帧的ack都应该是一样的
            TCPSegment tmp = q1.front();
            // cout << "syn is: "<< tmp.header().syn << endl;
            q1.pop();
            if (_receiver.ackno().has_value()) {
                tmp.header().ack = true;
                tmp.header().ackno = _receiver.ackno().value();
                tmp.header().win = _receiver.window_size();
                // cout << "syn is: "<< tmp.header().syn << endl;
                // cerr << "ackno is: " << tmp.header().ackno << endl;
            }
            _segments_out.push(tmp);
        }
    }
    else if (!_sender.stream_in().buffer_empty() || (!_sender.get_fin() && _sender.stream_in().eof())) {
        // 如果出站流还有没发出去的字节(即stream_in已经write还没有read，read就发出去了)，必须要发
        // 如果可以发送fin帧却从来没有发过fin，也要调用这个，可能就可以发送fin帧了
        _sender.fill_window();
        std::queue<TCPSegment> q = std::move(_sender.segments_out());
        size_t sz = q.size();
        if (sz > 0) {
            std::deque<TCPSegment> dq = std::move(_sender.segments_out_2());
            while (sz > 0) {
                dq.pop_back();
                sz--;
            }
            while (!dq.empty()) {
                TCPSegment dq_tmp = dq.front();
                dq.pop_front();
                _sender.segments_out_2().push_back(dq_tmp);
            }
            while (!q.empty()) {
                TCPSegment tmp = q.front();
                q.pop();
                if (_receiver.ackno().has_value()) {
                    tmp.header().ack = true;
                    tmp.header().ackno = _receiver.ackno().value();
                    tmp.header().win = _receiver.window_size();
                }
                _segments_out.push(tmp);
                _sender.segments_out_2().push_back(tmp);
            }
        }
        else {
            _sender.send_empty_segment();
            std::queue<TCPSegment> q1 = std::move(_sender.segments_out());
            // cout << "size of q is: "<< q.size() << endl;
            // 是不是这样处理并未实质的修改TCPSegment
            // 调用send_empty_segment()只会发送一个帧，调用fill_window可能发送多个帧，但这些帧的ack都应该是一样的
            TCPSegment tmp = q1.front();
            // cout << "syn is: "<< tmp.header().syn << endl;
            q1.pop();
            if (_receiver.ackno().has_value()) {
                tmp.header().ack = true;
                tmp.header().ackno = _receiver.ackno().value();
                tmp.header().win = _receiver.window_size();
                // cout << "syn is: "<< tmp.header().syn << endl;
                // cerr << "ackno is: " << tmp.header().ackno << endl;
            }
            _segments_out.push(tmp);
        }
    }
    else if (_receiver.ackno().has_value() && seg.header().seqno == _receiver.ackno().value() - 1) {
        // 对端发来的send_empty_segment
        _sender.send_empty_segment();
        // cout << "send empty_segment!" << endl;
        std::queue<TCPSegment> q = std::move(_sender.segments_out());
        // cout << "size of q is: "<< q.size() << endl;
        // 是不是这样处理并未实质的修改TCPSegment
        // 调用send_empty_segment()只会发送一个帧，调用fill_window可能发送多个帧，但这些帧的ack都应该是一样的
        TCPSegment tmp = q.front();
        // cout << "syn is: "<< tmp.header().syn << endl;
        q.pop();
        tmp.header().ack = true;
        tmp.header().ackno = _receiver.ackno().value();
        // cout << "syn is: "<< tmp.header().syn << endl;
        // cerr << "ackno is: " << tmp.header().ackno << endl;
        _segments_out.push(tmp);
    }
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        // 入站流结束，出站流还未达到eof，被动关闭
        // 应该是上层会不断调用tick，程序可以走到这里
        // cerr << "Inbound stream ended while outbound stream still not end！！" << endl;
        _linger_after_streams_finish = false;
    }
    if (_receiver.stream_out().input_ended() && sender_in_fin_acked()) {
        // 如何判定收到了fin的ack
        // 压根就没有执行到这里
        if (!_linger_after_streams_finish) {
            // cerr << "Clean shutdown of TCPConnection: " << "passive close!! "<< endl;
            _rst = true;
        }
        // 如何实现延迟？
        if (time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
            // cerr << "Clean shutdown of TCPConnection: " << "active close!! " << endl;
            _rst = true;
        }
    }
}

bool TCPConnection::active() const {
    if (_rst) return false;
    return true;
}

size_t TCPConnection::write(const string &data) {
    // 向出站流写入数据
    size_t res = _sender.stream_in().write(data);
    // 尝试发送新帧
    // fill_window可能还要改
    _sender.fill_window();
    // cerr << "the payload of tmp in segment_2 front : " << _sender.segments_out_2().front().payload().copy() << endl;
    // fill_window后面执行下面这一串
    // cerr << "the size of dq : " << _sender.segments_out_2().size() << endl;
    std::queue<TCPSegment> q = std::move(_sender.segments_out());
    std::deque<TCPSegment> dq = std::move(_sender.segments_out_2());
    // 记录这一轮添加了几个新帧，在dq中全部pop
    // 关于dq的逻辑是错的
    size_t sz = q.size();
    // cerr << "the size of dq : " << _sender.segments_out_2().size() << endl;
    while (sz > 0) {
        dq.pop_back();
        sz--;
    }
    // 得把最初的几个给放回去
    while (!dq.empty()) {
        TCPSegment dq_tmp = dq.front();
        dq.pop_front();
        _sender.segments_out_2().push_back(dq_tmp);
    }
    // cerr << "the size of dq : " << _sender.segments_out_2().size() << endl;
    while (!q.empty()) {
        // fill_window真的push了segment，我们认为q中最多有一个元素
        // 修改
        TCPSegment tmp = q.front();
        q.pop();
        if (_receiver.ackno().has_value()) {
            // 如果当前ack已经设置，
            // 如何将修改过的帧传入未完成的队列中
            // tmp是修改过的帧
            // 1 2 3 4
            // x x x 1 2 3 4
            // x x x
            tmp.header().ack = true;
            tmp.header().ackno = _receiver.ackno().value();
            tmp.header().win = _receiver.window_size();
        }
        _segments_out.push(tmp);
        _sender.segments_out_2().push_back(tmp);
        // cerr << "the size of dq : " << _sender.segments_out_2().size() << endl;
        // cerr << "the payload of tmp: " << tmp.payload().copy() << endl;
        // cerr << "the payload of tmp in segment_2 front : " << _sender.segments_out_2().front().payload().copy() << endl;
    }
    // cerr << "the size of dq: " << _sender.segments_out_2().size() << endl;
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        // 入站流结束，出站流还未达到eof，被动关闭
        // 应该是上层会不断调用tick，程序可以走到这里
        // cerr << "Inbound stream ended while outbound stream still not end！！" << endl;
        _linger_after_streams_finish = false;
    }
    if (_receiver.stream_out().input_ended() && sender_in_fin_acked()) {
        // 如何判定收到了fin的ack
        // 压根就没有执行到这里
        if (!_linger_after_streams_finish) {
            // cerr << "Clean shutdown of TCPConnection: " << "passive close!! "<< endl;
            _rst = true;
        }
        // 如何实现延迟？
        if (time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
            // cerr << "Clean shutdown of TCPConnection: " << "active close!! " << endl;
            _rst = true;
        }
    }
    return res;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
// 操作系统会定期调用tick
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // 这个函数是唯一知道时间流逝的
    // 告知TCPsender经过的时间
    _sender.tick(ms_since_last_tick);
    // 这时_segments_out可能有了内容，需要重传
    // cerr << "the size of q: " << _sender.segments_out().size() << endl;
    if (!_sender.segments_out().empty()) {
        std::queue<TCPSegment> q = std::move(_sender.segments_out());
        // cout << "size of q is: "<< q.size() << endl;
        // 是不是这样处理并未实质的修改TCPSegment
        // 调用send_empty_segment()只会发送一个帧，调用fill_window可能发送多个帧，但这些帧的ack都应该是一样的
        TCPSegment tmp = q.front();
        // cout << "syn is: "<< tmp.header().syn << endl;
        q.pop();
        // 是不是不应该无脑ack，如果此时value还没有设置
        if (_receiver.ackno().has_value()) {
            tmp.header().ack = true;
            tmp.header().ackno = _receiver.ackno().value();
            tmp.header().win = _receiver.window_size();
        }
        if (_sender.ret_Timer().get_rets() > TCPConfig::MAX_RETX_ATTEMPTS) {
            // 必然是一次重传后超过
            tmp.header().rst = true;
            _rst = true;
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
        }
        // cout << "syn is: "<< tmp.header().syn << endl;
        // cerr << "the payload of tmp: " << tmp.payload().copy() << endl;
        _segments_out.push(tmp);
    }
    _time_since_last_segment += ms_since_last_tick;
    // cerr << "ret_times: " << _sender.ret_Timer().get_rets() << endl;
    // 如果连续重新传输的次数超过上限TCPConfig:：MAX RETX ATTEMPTS，则中止连接，并向对等方发送重置段（设置了rst标志的空段）
    /*
    if (_sender.ret_Timer().get_rets() > TCPConfig::MAX_RETX_ATTEMPTS) {
        // 自己这一端先终止
        _rst = true;
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        // 向对端发送rst
        _sender.send_empty_segment();
        std::queue<TCPSegment> q = std::move(_sender.segments_out());
        TCPSegment tmp = q.front();
        q.pop();
        tmp.header().rst = true;
        _segments_out.push(tmp);
    }
     */
    // 如有必要，干净的结束连接
    // 满足条件1-3
    // 如果入站流在TCPConnection到达其出站流的EOF之前结束，则需要将此变量设置为false
    // 即先收到对端的fin帧，后达到出站流的EOF
    // 如果TCPConnection的入站流在TCPConnection发送fin段之前结束，那么在两个流都完成之后，TCPConnnection就不需要逗留
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        // 入站流结束，出站流还未达到eof，被动关闭
        // 应该是上层会不断调用tick，程序可以走到这里
        // cerr << "Inbound stream ended while outbound stream still not end！！" << endl;
        _linger_after_streams_finish = false;
    }
    // 判断是否满足条件1-3
    /*
    if (_sender.stream_in().input_ended()) {
        // cerr << "Outbound stream ended！！" << endl;
    }
    if (_sender.get_ackno() == _sender.next_seqno()) {
        // 这样判定收到了ack是否准确？
        // cerr << "all Outbound stream acked！！" << endl;
    }
     */
    // 把下面这里也改掉试试
    if (_receiver.stream_out().input_ended() && sender_in_fin_acked()) {
        // 如何判定收到了fin的ack
        // 压根就没有执行到这里
        if (!_linger_after_streams_finish) {
            // cerr << "Clean shutdown of TCPConnection: " << "passive close!! "<< endl;
            _rst = true;
        }
        // 如何实现延迟？
        if (time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
            // cerr << "Clean shutdown of TCPConnection: " << "active close!! " << endl;
            _rst = true;
        }
    }
}

void TCPConnection::end_input_stream() {
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        // 入站流结束，出站流还未达到eof，被动关闭
        // 应该是上层会不断调用tick，程序可以走到这里
        // cerr << "Inbound stream ended while outbound stream still not end！！" << endl;
        _linger_after_streams_finish = false;
    }
    // 测试文件中，调用这个就是要发送fin帧了
    _sender.stream_in().end_input();
    _sender.fill_window();
    std::queue<TCPSegment> q = std::move(_sender.segments_out());
    std::deque<TCPSegment> dq = std::move(_sender.segments_out_2());
    size_t sz = q.size();
    while (sz > 0) {
        dq.pop_back();
        sz--;
    }
    while (!dq.empty()) {
        TCPSegment dq_tmp = dq.front();
        dq.pop_front();
        _sender.segments_out_2().push_back(dq_tmp);
    }
    while (!q.empty()) {
        // fill_window真的push了segment，我们认为q中最多有一个元素
        // 修改
        TCPSegment tmp = q.front();
        q.pop();
        if (_receiver.ackno().has_value()) {
            // 如果当前ack已经设置，
            // 如何将修改过的帧传入未完成的队列中
            // tmp是修改过的帧
            // 1 2 3 4
            // x x x 1 2 3 4
            // x x x
            tmp.header().ack = true;
            tmp.header().ackno = _receiver.ackno().value();
            tmp.header().win = _receiver.window_size();
        }
        _segments_out.push(tmp);
        _sender.segments_out_2().push_back(tmp);
    }
    // cerr << "the size of dq: " << _sender.segments_out_2().size() << endl;
    if (_receiver.stream_out().input_ended() && sender_in_fin_acked()) {
        // 如何判定收到了fin的ack
        // 压根就没有执行到这里
        if (!_linger_after_streams_finish) {
            // cerr << "Clean shutdown of TCPConnection: " << "passive close!! "<< endl;
            _rst = true;
        }
        // 如何实现延迟？
        if (time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
            // cerr << "Clean shutdown of TCPConnection: " << "active close!! " << endl;
            _rst = true;
        }
    }
}

void TCPConnection::connect() {
    // 发送syn帧初始化连接
    // 在fill_window
    // _sender存的ackno是调用ack_received后收到对端的ackno
    // 己方组件segment需要提供的ackno在_receiver，fill_window不可能读取
    // cout << "start fill_window()" << endl;
    // cerr << "Start connect!!" << endl;
    _sender.fill_window();
    // cout << "end fill_window()" << endl;
    // TCPConnection和TCPSender下面都有一个出站队列，用哪个？要push到TCPconnection中才算发送
    // TCPSender的全部出列，重新push到这里的出站队列
    // 重传的帧怎么设置ack，重传的帧是从未完成的队列中获取的，只要未完成的队列中设置过ack，重传时直接push即可
    // 需要保证fill_window真的push了segment
    // 下面证必须要调用move改成右值
    std::queue<TCPSegment> q = std::move(_sender.segments_out());
    std::deque<TCPSegment> dq = std::move(_sender.segments_out_2());
    size_t sz = q.size();
    // cout << "size of q is: "<<sz << endl;
    while (sz > 0) {
        dq.pop_back();
        sz--;
    }
    while (!dq.empty()) {
        TCPSegment dq_tmp = dq.front();
        dq.pop_front();
        _sender.segments_out_2().push_back(dq_tmp);
    }
    while (!q.empty()) {
        // fill_window真的push了segment，我们认为q中最多有一个元素
        // 修改
        TCPSegment tmp = q.front();
        q.pop();
        if (_receiver.ackno().has_value()) {
            tmp.header().ack = true;
            tmp.header().ackno = _receiver.ackno().value();
            tmp.header().win = _receiver.window_size();
        }
        _segments_out.push(tmp);
        _sender.segments_out_2().push_back(tmp);
    }
    // cerr << "the size of dq: " << _sender.segments_out_2().size() << endl;
    // cout << "size of q is: "<<sz << endl;
    // cout << "size of q is: "<< _sender.segments_out().size()<< endl;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // Your code here: need to send a RST segment to the peer
            // 自己这一端先终止
            _rst = true;
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            // 向对端发送rst
            _sender.send_empty_segment();
            std::queue<TCPSegment> q = std::move(_sender.segments_out());
            q.back().header().rst = true;
            while (!q.empty()) {
                TCPSegment tmp = q.front();
                q.pop();
                _segments_out.push(tmp);
            }
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
