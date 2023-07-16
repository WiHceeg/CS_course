#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class TCPSender
{
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding? 注意这里已生成未发送的也算
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?

private:
  bool syn_generated_ = false;
  bool fin_generated_ = false;
  bool syn_sent_ = false;
  bool fin_sent_ = false;
  // bool syn_ack_ = false;
  // bool fin_ack_ = false;

  bool timer_started_ = false;
  bool maybe_send_by_tick_ = false;

  size_t ticks_duration_ = 0;
  uint64_t retransmission_timeout_ = 0;

  uint64_t next_seqno_ = 0;
  uint64_t window_size_ = 1;  // 由 receive 获得的

  uint64_t consecutive_retransmissions_ = 0;
  uint64_t sequence_numbers_in_flight_ = 0;

  std::queue<TCPSenderMessage> unsent_segments_;
  std::queue<TCPSenderMessage> outstanding_segments_;
  
};
