#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`


using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) :
    _output(capacity), _capacity(capacity),
    start(0), unassembled(0),
    eof1(false), last_idx(0),
    dq(capacity, ' '), dq2(capacity, 0) { }

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    // 查看index开头的data是否在dq的范围中
    // 前面都完整后，会调用write写入
    // dq的范围及当前内存的范围是从[start, end)
    // 但dq自己的范围一直是[0, cap)
    // dq[0]始终对应的是str[start]
    // dq[i]对应str[idx2]
    // 暂时没看懂eof有啥用
    // 写入没读的也在dq里面
    // 先根据已经读出的内容对dq进行处理，即让dq的前端由绿变蓝
    check();
    uint64_t end = start + _capacity;
    uint64_t sz = data.size();
    uint64_t idx1 = 0; // 0对应index
    uint64_t idx2 = index;
    uint64_t last_assemble = unassembled - 1;
    uint64_t last_dq = start - 1;
    if (eof)
        last_idx = index + sz - 1;
    while (idx2 < end && idx1 < sz) {
        // 先不考虑重复赋值
        // 压根没考虑data的长度
        if (idx2 >= start) {
            dq[idx2-start] = data[idx1];
            dq2[idx2-start] = 1;
            last_dq = idx2;
        }
        idx1++;
        idx2++;
    }
    if (last_idx == last_dq && eof)
        // 说明原字符串的最后一个字符已经放在dq中，是否装配不知道
        eof1 = true;
    string str;
    idx1 = unassembled - start;
    while (idx1 < _capacity) {
        if (dq2[idx1] == 0){
            unassembled = idx1 + start;
            break;
        }
        else {
            str += dq[idx1];
            last_assemble = idx1 + start;
        }
        idx1++;
        if (idx1 == _capacity)
            unassembled = idx1 + start;
    }
    // 整个string最后一个index是index + sz - 1
    //

    if (last_assemble == last_idx && eof1)
        _output.end_input();
    _output.write(str);
    //dq.resize(_capacity, ' ');
}

size_t StreamReassembler::unassembled_bytes() const {
    size_t res = 0;
    size_t idx = unassembled - start;
    while (idx < _capacity) {
        if (dq2[idx] == 1)
            res++;
        idx++;
    }
    return res;
}

bool StreamReassembler::empty() const { return unassembled == start; }

void StreamReassembler::check() {
    uint64_t bfsize = _output.buffer_size(); // bfsize是写但未读的大小，即绿色区间的长度
    uint64_t dif = unassembled - start;
    while (bfsize < dif) {
        dq.pop_front();
        dq2.pop_front();
        dif--;
    }
    // 上面操作会改变start，不会改变unassembled
    start = unassembled - dif;
    dq.resize(_capacity, ' ');
    dq2.resize(_capacity, 0); // 只会从末尾添加0，前面有效的字符(对应1)不会修改
}
