/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_GOSSIP_MESSAGE_H_
#define NODE_KEEPER_SRC_GOSSIP_MESSAGE_H_

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace gossip {
class Message {
 public:
  enum Type : uint8_t { kPush, kPull, kPullResponse };

  Message() = default;
  Message(Type type, const void *data, size_t size) {
    buffer_.reserve(kHeaderBytes + size);
    for (size_t i = 0; i < kHeaderLengthBytes; i++) {
      buffer_.push_back(size >> (8 * (kHeaderLengthBytes - i - 1)));
    }
    buffer_.push_back(type);
    auto begin = reinterpret_cast<const uint8_t *>(data);
    std::copy(begin, begin + size, std::back_inserter(buffer_));
  }
  Message(Message &&other) : buffer_(std::move(other.buffer_)) {}

  size_t Decode(const uint8_t *data, size_t size) {
    if (IsSatisfied()) {
      return 0;
    }
    size_t consumed = 0;
    if (!IsHeaderOut()) {
      consumed = DecodeHeader(data, data + size);
    }
    if (IsHeaderOut() && size > consumed) {
      consumed += DecodeBody(data + consumed, data + size);
    }
    return consumed;
  }

  bool IsSatisfied() const {
    return IsHeaderOut() && buffer_.size() == Length() + kHeaderBytes;
  }

  std::vector<uint8_t> Data() const {
    return std::vector<uint8_t>(buffer_.begin() + kHeaderBytes, buffer_.end());
  }

  void Reset() {
    /* intended to leave capacity unchanged here for performance */
    buffer_.clear();
  }

  const std::vector<uint8_t> &Encode() const { return buffer_; }

  Type Type() const {
    return static_cast<enum Type>(buffer_[kHeaderLengthBytes]);
  }

 private:
  size_t DecodeHeader(const uint8_t *begin, const uint8_t *end) {
    size_t consumed_bytes = 0;
    const size_t legacy = buffer_.size();
    const size_t current = end - begin;
    if (legacy + current < kHeaderBytes) {
      consumed_bytes = current;
    } else {
      consumed_bytes = kHeaderBytes - legacy;
    }
    std::copy(begin, begin + consumed_bytes, std::back_inserter(buffer_));
    if (Length() > 0) {
      buffer_.reserve(kHeaderBytes + Length());
    }
    return consumed_bytes;
  }

  size_t DecodeBody(const uint8_t *begin, const uint8_t *end) {
    const auto package_length = kHeaderBytes + Length();
    auto expected_bytes = package_length - buffer_.size();
    auto last = std::min(begin + expected_bytes, end);
    std::copy(begin, last, std::back_inserter(buffer_));
    return last - begin;
  }

  size_t Length() const {
    if (buffer_.size() < kHeaderBytes) {
      return 0;
    }
    size_t result = 0;
    for (size_t i = 0; i < kHeaderLengthBytes; i++) {
      result |= buffer_[i] << (8 * (kHeaderLengthBytes - i - 1));
    }
    return result;
  }

  bool IsHeaderOut() const { return buffer_.size() >= kHeaderBytes; }

 private:
  std::vector<uint8_t> buffer_;

  static const size_t kHeaderBytes = 5;
  static const size_t kHeaderLengthBytes = 4;
};
}  // namespace gossip
#endif  // NODE_KEEPER_SRC_GOSSIP_MESSAGE_H_
