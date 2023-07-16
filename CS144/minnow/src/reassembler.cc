#include "reassembler.hh"
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.

  if (data.empty()) {
    if (is_last_substring) {
      output.close();
    }
    return;
  }
  first_unpoped_index_ = output.reader().bytes_popped();
  first_unassembled_index_ = first_unpoped_index_ + output.reader().bytes_buffered();
  first_unacceptable_index_ = first_unassembled_index_ + output.writer().available_capacity();
  if (is_last_substring) {
    eof_idx_ = first_index + data.size();
  }
  if (first_index + data.size() <= first_unassembled_index_ || first_index >= first_unacceptable_index_) {
    return;
  }
  preprocess(first_index, data);  // 预处理后，first_index、data 可能也会改变
  merge(first_index, data);  // merge 后全是断的
  auto begin_iter = unassembled_strings_.begin();

  if (first_unassembled_index_ == begin_iter->first) {
    output.push(begin_iter->second);
    bytes_pending_ -= begin_iter->second.size();
    unassembled_strings_.erase(begin_iter);
    if (eof_idx_ && output.writer().bytes_pushed() == eof_idx_) {
      output.close();
    }
  }
}

void Reassembler::preprocess(uint64_t &first_index, string &data) {
  // 预处理后，first_index、data 可能也会改变

  // 去头
  if (first_index < first_unassembled_index_ && first_index + data.size() > first_unassembled_index_) {
    uint64_t start_pos = first_unassembled_index_ - first_index;
    data.erase(0, start_pos);
    first_index = first_unassembled_index_;
  }

  // 去尾
  if (first_index + data.size() > first_unacceptable_index_) {
    data.erase(first_unacceptable_index_ - first_index);
  }
}

void Reassembler::merge(uint64_t first_index, string &data) {
  // 不用放入
  if (unassembled_strings_.contains(first_index)) {
    if (data.size() <= unassembled_strings_[first_index].size()) {
      return;
    }
    bytes_pending_ -= unassembled_strings_[first_index].size();  // 如果发生替换，先减去被替换掉的，之后再加上本体
  }

  // 与前一个合并
  unassembled_strings_[first_index] = data;
  bytes_pending_ += data.size();
  auto first_index_iter = unassembled_strings_.find(first_index);
  auto unmerged_iter = first_index_iter;  // 无论是否与前一个合并，都使用这个保存将要处理的迭代器
  if (first_index_iter != unassembled_strings_.begin()) {
    auto pre_iter = std::prev(first_index_iter, 1);
    if (pre_iter->first + pre_iter->second.size() >= first_index) {
      if (pre_iter->first + pre_iter->second.size() < first_index + data.size()) {
        uint64_t cut_length = pre_iter->first + pre_iter->second.size() - first_index_iter->first;
        bytes_pending_ -= cut_length;
        pre_iter->second.append(first_index_iter->second, cut_length);
      } else {
        bytes_pending_ -= data.size();
      }
      unassembled_strings_.erase(first_index_iter);
      unmerged_iter = pre_iter;  // 将要处理的迭代器
    }
  }

  // 与后续的合并
  while(unmerged_iter != std::prev(unassembled_strings_.end())) {
    auto next_iter = std::next(unmerged_iter);
    if (unmerged_iter->first + unmerged_iter->second.size() >= next_iter->first) {
      // 部分重叠合并后 break
      if (unmerged_iter->first + unmerged_iter->second.size() < next_iter->first + next_iter->second.size()) {
        uint64_t cut_length = unmerged_iter->first + unmerged_iter->second.size() - next_iter->first;
        bytes_pending_ -= cut_length;
        unmerged_iter->second.append(next_iter->second, cut_length);
        unassembled_strings_.erase(next_iter);
        break;
      } else {
        // 完全重叠删掉后继续循环
        bytes_pending_ -= next_iter->second.size();
        unassembled_strings_.erase(next_iter);
      }
    } else {
      break;
    }
  }



}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
