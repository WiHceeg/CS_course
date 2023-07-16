#include <stdexcept>

#include "byte_stream.hh"
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  // Your code here.
  if (is_closed() || has_error_) {
    return;
  }
  for(const char& c: data) {
    if (buffer_.size() < capacity_) {
      buffer_.push_back(c);
      bytes_pushed_++;
    }
  }
}

void Writer::close()
{
  // Your code here.
  is_open_ = false;
}

void Writer::set_error()
{
  // Your code here.
  has_error_ = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return !is_open_;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  // Your code here.
  if (buffer_.size() == 0 || has_error()) {
    return "";
  }
  return string_view(&buffer_.front(), 1);
}

bool Reader::is_finished() const
{
  // Your code here.
  return (!is_open_) && buffer_.size() == 0;
}

bool Reader::has_error() const
{
  // Your code here.
  return has_error_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.

    const std::size_t size = std::min(len, buffer_.size());
    buffer_.erase(buffer_.cbegin(), buffer_.cbegin() + size);
    bytes_popped_ += size;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer_.size();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}
