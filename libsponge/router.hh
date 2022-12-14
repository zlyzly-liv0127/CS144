#ifndef SPONGE_LIBSPONGE_ROUTER_HH
#define SPONGE_LIBSPONGE_ROUTER_HH

#include "network_interface.hh"

#include <optional>
#include <queue>

//! \brief A wrapper for NetworkInterface that makes the host-side
//! interface asynchronous: instead of returning received datagrams
//! immediately (from the `recv_frame` method), it stores them for
//! later retrieval. Otherwise, behaves identically to the underlying
//! implementation of NetworkInterface.
class AsyncNetworkInterface : public NetworkInterface {
    std::queue<InternetDatagram> _datagrams_out{};

  public:
    using NetworkInterface::NetworkInterface;

    //! Construct from a NetworkInterface
    AsyncNetworkInterface(NetworkInterface &&interface) : NetworkInterface(interface) {}

    //! \brief Receives and Ethernet frame and responds appropriately.

    //! - If type is IPv4, pushes to the `datagrams_out` queue for later retrieval by the owner.
    //! - If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
    //! - If type is ARP reply, learn a mapping from the "target" fields.
    //!
    //! \param[in] frame the incoming Ethernet frame
    void recv_frame(const EthernetFrame &frame) {
        auto optional_dgram = NetworkInterface::recv_frame(frame);
        if (optional_dgram.has_value()) {
            _datagrams_out.push(std::move(optional_dgram.value()));
        }
    };

    //! Access queue of Internet datagrams that have been received
    std::queue<InternetDatagram> &datagrams_out() { return _datagrams_out; }
};
// 需要自己实现一个结构体
struct RouterTable{
    uint32_t _route_prefix;
    uint8_t _prefix_length;
    std::optional<Address> _next_hop;
    size_t _interface_num;
    RouterTable():_route_prefix(0), _prefix_length(0), _next_hop(), _interface_num(0) {}
    RouterTable(uint32_t route_prefix,
                uint8_t prefix_length,
                std::optional<Address> next_hop,
                size_t interface_num) :
                _route_prefix(route_prefix),
                _prefix_length(prefix_length),
                _next_hop(next_hop),
                _interface_num(interface_num) {}

};


//! \brief A router that has multiple network interfaces and
//! performs longest-prefix-match routing between them.
class Router {
    //! The router's collection of network interfaces
    // 一个路由器有多个网口
    std::vector<AsyncNetworkInterface> _interfaces{};

    //! Send a single datagram from the appropriate outbound interface to the next hop,
    //! as specified by the route with the longest prefix_length that matches the
    //! datagram's destination address.
    // 将单个数据报从适当的出站接口发送到下一跳，如路由指定的，该路由具有与数据报的目的地址匹配的最长前缀_length。
    void route_one_datagram(InternetDatagram &dgram);

    // 需要维护路由表，采用什么数据结构？
    std::vector<RouterTable> _table{};

  public:
    //! Add an interface to the router
    //! \param[in] interface an already-constructed network interface
    //! \returns The index of the interface after it has been added to the router
    size_t add_interface(AsyncNetworkInterface &&interface) {
        _interfaces.push_back(std::move(interface));
        return _interfaces.size() - 1;
    }

    //! Access an interface by index
    AsyncNetworkInterface &interface(const size_t N) { return _interfaces.at(N); }

    //! Add a route (a forwarding rule)
    void add_route(const uint32_t route_prefix,
                   const uint8_t prefix_length,
                   const std::optional<Address> next_hop,
                   const size_t interface_num);

    //! Route packets between the interfaces
    void route();
};

#endif  // SPONGE_LIBSPONGE_ROUTER_HH
