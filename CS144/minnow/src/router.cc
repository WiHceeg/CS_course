#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  route_table_[prefix_length][route_prefix] = std::make_pair(next_hop, interface_num);
}

void Router::route() {
  for (AsyncNetworkInterface &interface: interfaces_) {
    while (true) {
      std::optional<InternetDatagram> dgram = interface.maybe_receive();
      if (dgram.has_value()) {
        route_one_datagram(dgram.value());
      } else {
        break;
      }
    }
  }
}

void Router::route_one_datagram(InternetDatagram &dgram) {
  if (dgram.header.ttl <= 1) {
    return;
  }
  uint32_t &dst = dgram.header.dst;
  uint32_t mask = UINT32_MAX;
  for (int i = 32; i >= 0; --i) {
    uint32_t prefix = mask & dst;
    if (route_table_[i].contains(prefix)) {
      --dgram.header.ttl;
      dgram.header.compute_checksum();  // ttl修改后要重新计算
      auto& [next_hop, interface_num] = route_table_[i][prefix];
      interfaces_[interface_num].send_datagram(dgram, next_hop.value_or(Address::from_ipv4_numeric(dst)));
      return;
    } else {
      mask <<= 1;
    }
  }
}