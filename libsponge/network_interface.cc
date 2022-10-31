#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>
#include <vector>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    // 将数据报转换为以太网帧并发送
    // 将下一跳的IP地址转化为整数
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    // 目标以太网地址已知，直接发送
    // DUMMY_CODE(dgram, next_hop, next_hop_ip);
    EthernetFrame ef;
    bool flag = false;
    if (_mp.find(next_hop_ip) != _mp.end()) {
        std::pair tmp = _mp[next_hop_ip];
        if (_all_tm - tmp.first >= 30 * 1000) {
            // 映射超时
            cerr << "map out of time!" << endl;
            flag = true;
        }
    }
    else {
        flag = true;
    }
    if (!flag) {
        // 组帧并发送
        ef.payload() = dgram.serialize();
        ef.header().type = EthernetHeader::TYPE_IPv4;
        ef.header().src = _ethernet_address;
        ef.header().dst = _mp[next_hop_ip].second;
        // 发送以太网帧
        _frames_out.push(ef);
    }

    // 以太网地址未知(在映射中找不到)，广播ARP请求，并将IP数据报排队，以便在收到ARP回复后发送
    // 5s内只对同一个IP地址发送一个ARP
    else {
        // 怎么发ARP报文？
        if (_mp2.find(next_hop_ip) != _mp2.end()){
            size_t _last_time = _mp2[next_hop_ip];
            if (_all_tm - _last_time < 5 * 1000) {
                // 和上一次间隔时间太近，直接返回
                return;
            }
        }
        ef.header().type = EthernetHeader::TYPE_ARP;
        ef.header().src = _ethernet_address;
        // 广播的目的地址是网关？
        ef.header().dst = ETHERNET_BROADCAST;
        // Payload是arpmessage
        ARPMessage _arp;
        _arp.opcode = ARPMessage::OPCODE_REQUEST;
        _arp.hardware_type = ARPMessage::TYPE_ETHERNET;
        _arp.protocol_type = EthernetHeader::TYPE_IPv4;
        _arp.sender_ethernet_address = _ethernet_address;
        _arp.sender_ip_address = _ip_address.ipv4_numeric();
        // 目的端的以太网地址不需要填
        _arp.target_ip_address = next_hop_ip;
        // 不知道映射的数据报排队，排队有意义吗？
        // 要记录上一次请求该IP的以太网地址的时间
        // 进入q排队的是IP数据报和对端的IP地址，一旦收到ARP回复就发送以太帧
        // _mp2存的是ip地址和最近一次请求的时间
        _q.push(make_pair(dgram, next_hop_ip));
        _mp2[next_hop_ip] = _all_tm;
        // 发送ARP
        ef.payload() = _arp.serialize();
        _frames_out.push(ef);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().dst == _ethernet_address || frame.header().dst == ETHERNET_BROADCAST){
        // 这种情况才会进行处理
        if (frame.header().type == EthernetHeader::TYPE_IPv4) {
            // ipv4帧
            InternetDatagram res;
            if (res.parse(frame.payload()) == ParseResult::NoError) {
                // parse是数据帧初始化的一种手段
                return res;
            }
            else return {};
        }
        else if (frame.header().type == EthernetHeader::TYPE_ARP) {
            // arp
            ARPMessage _arp;
            if (_arp.parse(frame.payload()) == ParseResult::NoError) {
                // parse是数据帧初始化的一种手段
                // 判定ARP请求帧还是回复帧
                if (_arp.opcode == ARPMessage::OPCODE_REPLY) {
                    // 回复帧
                    // 记录相应的映射
                    // 这里to_string后映射相同，本来的值应该是地址值，是不同的
                    uint32_t _ipaddress = _arp.sender_ip_address;
                    // cerr << "the received ipadderss : " << _ipaddress.to_string() << endl;
                    // cerr << "the received ipaddress : " << _arp.sender_ip_address << endl;
                    _mp[_ipaddress] = make_pair(_all_tm, _arp.sender_ethernet_address);
                    // 循环遍历头部的帧
                    while (!_q.empty()) {
                        // cerr << "size of _q: " << _q.size() << endl;
                        uint32_t _tmp_ip = _q.front().second;
                        // cerr << "the front of _q: " << _tmp_ip.to_string() << endl;
                        if (_mp.find(_tmp_ip) != _mp.end()) {
                            // 找到映射，组帧并发送
                            // 这里就不检测映射超时了，感觉不用
                            EthernetFrame ef;
                            ef.payload() = _q.front().first.serialize();
                            ef.header().type = EthernetHeader::TYPE_IPv4;
                            ef.header().src = _ethernet_address;
                            ef.header().dst = _mp[_tmp_ip].second;
                            // 发送以太网帧
                            cerr << "send ethernet datagram!" << endl;
                            _frames_out.push(ef);
                            _q.pop();
                            _mp2.erase(_tmp_ip);
                        }
                        else break;
                    }
                }
                else if (_arp.opcode == ARPMessage::OPCODE_REQUEST){
                    // 请求帧
                    // 记录相应的映射
                    uint32_t _ipaddress = _arp.sender_ip_address;
                    _mp[_ipaddress] = make_pair(_all_tm, _arp.sender_ethernet_address);
                    // 循环遍历头部的帧
                    while (!_q.empty()) {
                        uint32_t _tmp_ip = _q.front().second;
                        if (_mp.find(_tmp_ip) != _mp.end()) {
                            // 找到映射，组帧并发送
                            // 这里就不检测映射超时了，感觉不用
                            EthernetFrame ef;
                            ef.payload() = _q.front().first.serialize();
                            ef.header().type = EthernetHeader::TYPE_IPv4;
                            ef.header().src = _ethernet_address;
                            ef.header().dst = _mp[_tmp_ip].second;
                            // 发送以太网帧
                            cerr << "send ethernet datagram!" << endl;
                            _frames_out.push(ef);
                            _q.pop();
                            _mp2.erase(_tmp_ip);
                        }
                        else break;
                    }
                    EthernetFrame ef;
                    // 需要回复arp请求报文
                    // 得是发给自己的采用回复
                    if (_arp.target_ip_address != _ip_address.ipv4_numeric()){
                        return {};
                    }
                    // 不能向网关广播了
                    ef.header().type = EthernetHeader::TYPE_ARP;
                    ef.header().src = _ethernet_address;
                    ef.header().dst = frame.header().src;
                    // Payload是arpmessage
                    ARPMessage _arp_reply;
                    _arp_reply.opcode = ARPMessage::OPCODE_REPLY;
                    _arp_reply.hardware_type = ARPMessage::TYPE_ETHERNET;
                    _arp_reply.protocol_type = EthernetHeader::TYPE_IPv4;
                    _arp_reply.sender_ethernet_address = _ethernet_address;
                    _arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
                    // 目的端的以太网地址不需要填
                    _arp_reply.target_ip_address = _arp.sender_ip_address;
                    _arp_reply.target_ethernet_address = _arp.sender_ethernet_address;
                    // 发送ARP
                    ef.payload() = _arp_reply.serialize();
                    _frames_out.push(ef);
                }
            }
        }
        return {};
    }
    else return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // 删除所有过期的映射
    _all_tm += ms_since_last_tick;
    // 感觉这样实现时有问题的
    std::vector<uint32_t> _rm;
    for (auto &a: _mp) {
        uint32_t _ip_addr = a.first;
        if (_all_tm - a.second.first >= 30 * 1000)
            _rm.push_back(_ip_addr);
    }
    for (auto &b: _rm){
        _mp.erase(b);
    }
    // DUMMY_CODE(ms_since_last_tick);
}
