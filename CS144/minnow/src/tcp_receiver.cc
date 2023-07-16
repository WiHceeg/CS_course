#include "tcp_receiver.hh"
#include <iostream>
#include <format>

using namespace std;

// 注意，syn 和 fin 都占 1 个 seq
void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if (!received_syn_) {
    if (message.SYN) {
      received_syn_ = true;
      syn_seqno_ = message.seqno;
      reassembler.insert(
        (message.seqno + 1).unwrap(syn_seqno_, inbound_stream.writer().bytes_pushed()) - 1,  // 如果有syn 和 payload 同时出现，message.seqno 是 syn 的，而 payload 的 seqno 大 1
        message.payload,
        message.FIN,
        inbound_stream
      );
      if (message.FIN) {
        received_fin_ = true;
      }
    } else {
      return;
    }
  } else {
    reassembler.insert(
      message.seqno.unwrap(syn_seqno_, inbound_stream.writer().bytes_pushed()) - 1,
      message.payload,
      message.FIN,
      inbound_stream
    );
    if (message.FIN) {
      received_fin_ = true;
    }
  }

}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage msg;
  if (!received_syn_) {
    msg.ackno = nullopt;
  } else {
    if (received_fin_) {
      if (inbound_stream.is_closed()) {
        msg.ackno = syn_seqno_ + 1 + inbound_stream.bytes_pushed() + received_fin_;
      } else {
        // 如果没有 closed，说明中间断了
        msg.ackno = syn_seqno_ + 1 + inbound_stream.bytes_pushed();
      }
    } else {
      msg.ackno = syn_seqno_ + 1 + inbound_stream.bytes_pushed();
    }
  }
  msg.window_size = std::min(inbound_stream.available_capacity(), static_cast<uint64_t>(UINT16_MAX));
  return msg;
}
