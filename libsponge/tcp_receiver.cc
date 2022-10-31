#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.


using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // 核心处理函数
    TCPHeader hd = seg.header();
    // 目前我们认为SYN和FIN都只会出现一次
    // 收到的一个数据报的seqno是起始的index，payload中有几个字节就加几
    // 先这样理解看看对不对吧
    if (hd.syn) {
        // 接收到了syn帧
        _syn = true;
        isn = hd.seqno;
    }
    string payload = seg.payload().copy();
    // 调用push_substring
    if (_syn) {
        // 没收到syn，收到其他的数据帧没意义
        // 下面得到的是流的index，即从0开始的
        uint64_t checkpoint = stream_out().bytes_written();
        // idx是绝对序列号，和流的index还有微小的区别，需要减1
        // 要看是不是这一轮收到的syn帧
        size_t idx;
        if (hd.syn)
            idx = unwrap(hd.seqno + 1, isn, checkpoint) - 1;
        else
            idx = unwrap(hd.seqno, isn, checkpoint) - 1;
        // 直接push即可，如果是fin帧，就要加上eof
        _reassembler.push_substring(payload, idx, hd.fin);
    }

}

optional<WrappingInt32> TCPReceiver::ackno() const {
    // 已经设置syn
    if (_syn) {
        // 流的index需要加1才是绝对序列号
        if (stream_out().input_ended())
            // 收到fin，还要再加1
            return wrap(stream_out().bytes_written() + 2, isn);
        return wrap(stream_out().bytes_written() + 1, isn);
    }
    // 未设置isn返回空
    return {};
}

size_t TCPReceiver::window_size() const {
    return stream_out().remaining_capacity();
}
