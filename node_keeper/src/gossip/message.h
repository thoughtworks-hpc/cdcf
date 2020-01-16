/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_GOSSIP_MESSAGE_H_
#define NODE_KEEPER_SRC_GOSSIP_MESSAGE_H_

#include <algorithm>
#include <cstdint>
#include <vector>

namespace gossip {
class Message {
 public:
  static const size_t kHeaderLength = 4;

  Message() = default;
  Message(const uint8_t *data, size_t size) {
    buffer_.reserve(kHeaderLength + size);
    for (size_t i = 0; i < kHeaderLength; i++) {
      buffer_.push_back(size >> (8 * (kHeaderLength - i - 1)));
    }
    std::copy(data, data + size, std::back_inserter(buffer_));
  }

  size_t Decode(const uint8_t *data, size_t size) {
    if (IsSatisfied()) {
      return 0;
    }
    size_t consumed = 0;
    if (!IsHeaderOut()) {
      consumed = decodeHeader(data, data + size);
    }
    if (IsHeaderOut() && size > consumed) {
      consumed += decodeBody(data + consumed, data + size);
    }
    return consumed;
  }

  bool IsSatisfied() const {
    return IsHeaderOut() && buffer_.size() == Length() + kHeaderLength;
  }

  std::vector<uint8_t> Data() const {
    return std::vector<uint8_t>(buffer_.begin() + kHeaderLength, buffer_.end());
  }

  void Reset() {
    /* intended to leave capacity unchanged here for performance */
    buffer_.clear();
  }

  const std::vector<uint8_t> &Encode() const { return buffer_; }

 private:
  size_t decodeHeader(const uint8_t *begin, const uint8_t *end) {
    size_t consumedBytes = 0;
    const size_t size = end - begin;
    if (buffer_.size() + size < kHeaderLength) {
      consumedBytes = size;
    } else {
      consumedBytes = kHeaderLength - buffer_.size();
    }
    std::copy(begin, begin + consumedBytes, std::back_inserter(buffer_));
    if (Length() > 0) {
      buffer_.reserve(kHeaderLength + Length());
    }
    return consumedBytes;
  }

  size_t decodeBody(const uint8_t *begin, const uint8_t *end) {
    auto expectedBytes = Length() + kHeaderLength - buffer_.size();
    auto last = std::min(begin + expectedBytes, end);
    std::copy(begin, last, std::back_inserter(buffer_));
    return last - begin;
  }

  size_t Length() const {
    if (buffer_.size() < kHeaderLength) {
      return 0;
    }
    size_t result = 0;
    for (size_t i = 0; i < kHeaderLength; i++) {
      result |= buffer_[i] << (8 * (kHeaderLength - i - 1));
    }
    return result;
  }

  bool IsHeaderOut() const { return buffer_.size() >= kHeaderLength; }

 private:
  std::vector<uint8_t> buffer_;
};
}  // namespace gossip
#endif  // NODE_KEEPER_SRC_GOSSIP_MESSAGE_H_
