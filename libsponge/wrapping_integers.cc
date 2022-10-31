#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // 64位索引转为32位序列号
    // 用到给的辅助函数
    // 常量取mod或者静态转换
    uint32_t res = static_cast<uint32_t> (n);
    return isn + res;
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // 序列号转为绝对序列号，如果没有checkpoint，会有很多对应的绝对序列号
    // 返回离checkpoint最近的绝对序列号
    // 给定的n就是一个32位的序列号，需要注意的只是n和isn的距离始终不变，即offset
    // dif就是这个offset，范围是[-2147483648, 2147483647]将其转化为uint32，便于后面的判定，但是这样改变了偏置，不可取
    int32_t dif = n - isn;
    const uint64_t a = 4294967296;
    uint32_t b = static_cast<uint32_t>(checkpoint / a);
    uint32_t remainder = static_cast<uint32_t>(checkpoint % a); // 这一定是正值,介于[0, 4294967295]之间
    // 两数的差值从[-2^31, 3 * 2 ^31]之间 差值在[-2^31,2^31]取本身
    // 距离刚好是2^31怎么办 2147483648
    uint64_t res;
    int64_t dif2 = static_cast<int64_t>(remainder) - static_cast<int64_t>(dif);
    // 看n和x离的是否足够近
    if (dif2 <= 2147483648) {
        if (dif < 0 && b == 0) {
            res = (b + 1) * a + dif;
        }
        else
            res = b * a + dif;
    }
    else {
        res = (b + 1) * a + dif;
    }
    return res;
}
