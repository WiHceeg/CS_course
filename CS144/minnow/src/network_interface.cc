#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t next_hop_ip = next_hop.ipv4_numeric();
  auto iter = arp_table_.find(next_hop_ip);
  if (iter != arp_table_.end()) {
    EthernetFrame eth_frame;
    // 表里有的话，找到缓存的 MAC 地址直接发送IP包
    eth_frame.header = {
      .dst = iter->second.first,
      .src = ethernet_address_,
      .type = EthernetHeader::TYPE_IPv4,
    };
    eth_frame.payload = serialize(dgram);
    frame_out_.push(eth_frame);
  } else {  // 表里没有的话。
    // 如果前 5 秒没发过请求就发送请求
    if (!waiting_arp_reply_dgram_.contains(next_hop_ip)) {
      ARPMessage arp_request = {
        .opcode = ARPMessage::OPCODE_REQUEST,
        .sender_ethernet_address = ethernet_address_,
        .sender_ip_address = ip_address_.ipv4_numeric(),
        // 需要获得 target_ethernet_address
        .target_ip_address = next_hop_ip,
      };
      EthernetFrame eth_frame;
      eth_frame.header = {
        .dst = ETHERNET_BROADCAST,
        .src = ethernet_address_,
        .type = EthernetHeader::TYPE_ARP
      };
      eth_frame.payload = serialize(arp_request);
      frame_out_.push(eth_frame);
      waiting_arp_reply_dgram_[next_hop_ip] = make_pair(list<InternetDatagram>({dgram}), ARP_REQUEST_DEFAULT_TTL);
    } else {
      waiting_arp_reply_dgram_[next_hop_ip].first.push_back(dgram);
    }

  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if (frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST) {
    return nullopt;
  }
  if (frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram ret;
    if (parse(ret, frame.payload)) {
      return ret;
    } else {
      return nullopt;
    }
  } else if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arp_msg;
    if (parse(arp_msg, frame.payload)) {
      EthernetAddress &src_eth_addr = arp_msg.sender_ethernet_address;
      uint32_t &src_ip_addr = arp_msg.sender_ip_address;
      uint32_t &dst_ip_addr = arp_msg.target_ip_address;
      arp_table_[src_ip_addr] = make_pair(src_eth_addr, IP_MAC_PAIR_DEFAULT_TTL);
      if (waiting_arp_reply_dgram_.contains(src_ip_addr)) {
        for (InternetDatagram &dgram: waiting_arp_reply_dgram_[src_ip_addr].first) {
          EthernetFrame eth_frame;
          eth_frame.header = {
            .dst = src_eth_addr,
            .src = ethernet_address_,
            .type = EthernetHeader::TYPE_IPv4
          };
          eth_frame.payload = serialize(dgram);
          frame_out_.push(eth_frame);
        }
        waiting_arp_reply_dgram_.erase(src_ip_addr);
      }

      if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST && dst_ip_addr == ip_address_.ipv4_numeric()) {
        ARPMessage arp_reply = {
          .opcode = ARPMessage::OPCODE_REPLY,
          .sender_ethernet_address = ethernet_address_,
          .sender_ip_address = ip_address_.ipv4_numeric(),
          .target_ethernet_address = src_eth_addr,
          .target_ip_address = src_ip_addr,
        };
        EthernetFrame eth_frame;
        eth_frame.header = {
          .dst = src_eth_addr,
          .src = ethernet_address_,
          .type = EthernetHeader::TYPE_ARP,
        };
        eth_frame.payload = serialize(arp_reply);
        frame_out_.push(eth_frame);
      }
      return nullopt;
    } else {
      return nullopt;
    }
  } else {
    return nullopt;
  }
  
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  for (auto iter = arp_table_.begin(); iter != arp_table_.end(); ) {
    if (iter->second.second <= ms_since_last_tick) {
      iter = arp_table_.erase(iter);
    } else {
      iter->second.second -= ms_since_last_tick;
      iter++;
    }
  }

  for (auto iter = waiting_arp_reply_dgram_.begin(); iter != waiting_arp_reply_dgram_.end(); iter++) {
    if (iter->second.second <= ms_since_last_tick) {
      ARPMessage arp_request = {
        .opcode = ARPMessage::OPCODE_REQUEST,
        .sender_ethernet_address = ethernet_address_,
        .sender_ip_address = ip_address_.ipv4_numeric(),
        // 需要获得 target_ethernet_address
        .target_ip_address = iter->first,
      };
      EthernetFrame eth_frame;
      eth_frame.header = {
        .dst = ETHERNET_BROADCAST,
        .src = ethernet_address_,
        .type = EthernetHeader::TYPE_ARP
      };
      eth_frame.payload = serialize(arp_request);
      frame_out_.push(eth_frame);
      
      iter->second.second = ARP_REQUEST_DEFAULT_TTL;
    } else {
      iter->second.second -= ms_since_last_tick;
    }
  }


}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if (frame_out_.empty()) {
    return nullopt;
  } else {
    EthernetFrame ret = std::move(frame_out_.front());
    frame_out_.pop();
    return ret;
  }
}
