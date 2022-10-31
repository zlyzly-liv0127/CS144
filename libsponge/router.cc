#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";
    // 目的ip是route_prefix，子网掩码长度为prefix_length，这样的目的ip转发到网口interface_num，下一跳ip是
    // DUMMY_CODE(route_prefix, prefix_length, next_hop, interface_num);
    // Your code here.
    // 什么数据结构存储路由表，目的ip映射到网口和下一跳ip的pair
    // 如何对这些key实施最长匹配原则
    // 1.1.1.1/16 和 1.1.1.1/24
    // 1.1.1.12匹配后者，1.1.3.11匹配前者
    RouterTable _route(route_prefix, prefix_length, next_hop, interface_num);
    _table.push_back(_route);
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // DUMMY_CODE(dgram);
    // Your code here.
    // 数据报有目的地址 通过查找路由表得到该数据报的网口索引和下一跳地址
    // 每个网口对应一个子网
    // 移位操作会出现的问题
    // 注意要减少ttl
    // 最长匹配原则的实现
    uint32_t _dst = dgram.header().dst;
    uint8_t _ttl = dgram.header().ttl;
    if (_ttl == 0)
        return;
    dgram.header().ttl -= 1;
    if (dgram.header().ttl == 0)
        return;
    RouterTable _max;
    bool flag = false;
    // _max._prefix_length = 0;
    for (auto &route: _table){
        //
        if (route._prefix_length == 0 && _max._prefix_length == 0) {
            // 考虑0的特殊情况
            uint32_t _mask = 0;
            if ((_dst & _mask) == route._route_prefix) {
                flag = true;
                _max._route_prefix = route._route_prefix;
                _max._prefix_length = route._prefix_length;
                _max._next_hop = route._next_hop;
                _max._interface_num = route._interface_num;
            }
        }
        if (route._prefix_length > _max._prefix_length) {
            // 这种情况下才需要考虑
            uint32_t _mask = 0xFFFFFFFF << (32 - route._prefix_length);
            if ((_dst & _mask) == route._route_prefix) {
                //
                flag = true;
                _max._route_prefix = route._route_prefix;
                _max._prefix_length = route._prefix_length;
                _max._next_hop = route._next_hop;
                _max._interface_num = route._interface_num;
            }
        }
    }
    // 得到最终匹配的结果
    if (!flag)
        return;
    if (_max._next_hop.has_value()) {
        interface(_max._interface_num).send_datagram(dgram, _max._next_hop.value());
    }
    else {
        // 说明下一跳就是当前子网
        interface(_max._interface_num).send_datagram(dgram, Address::from_ipv4_numeric(dgram.header().dst));
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
