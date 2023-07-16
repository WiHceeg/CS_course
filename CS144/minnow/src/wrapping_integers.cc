#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32 { static_cast<uint32_t>((zero_point.raw_value_ +  n) & ((0ul - 1ul) >> 32))};
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  uint64_t offset = static_cast<uint64_t>(this->raw_value_ - zero_point.raw_value_);
  if (checkpoint < offset) {
    return offset;
  }
  uint64_t checkpoint_tail = ((0ul - 1ul) >> 32) & checkpoint;
  uint64_t checkpoint_left_zero = ((0ul - 1ul) << 32) & checkpoint;
  if (checkpoint_tail < offset) {
    if (offset - checkpoint_tail <= checkpoint_tail + (1ul << 32) - offset) {
      return checkpoint_left_zero + offset;
    } else {
      return checkpoint_left_zero + offset - (1ul << 32);
    }
  } else {
    if (checkpoint_tail - offset <= offset + (1ul << 32) - checkpoint_tail) {
      return checkpoint_left_zero + offset;
    } else {
      return checkpoint_left_zero + offset + (1ul << 32);
    }
  }  
}
