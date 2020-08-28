/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_GOSSIP_PULL_SESSION_H_
#define NODE_KEEPER_SRC_GOSSIP_PULL_SESSION_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <asio.hpp>

#include "src/gossip.h"

namespace gossip {
class PullSession : public std::enable_shared_from_this<PullSession> {
 public:
  PullSession(asio::io_context *context, const std::string &host,
              const std::string &port)
      : socket_(*context) {
    asio::ip::tcp::resolver resolver(*context);
    auto endpoints = resolver.resolve(host, port);
    asio::connect(socket_, endpoints);
  }

  void Cancel() { socket_.close(); }

  Pullable::PullResult Request(const void *data, size_t size,
                               Pullable::DidPullHandler did_pull = nullptr) {
    did_pull_ = did_pull;
    auto out = Message(Message::Type::kPull, data, size).Encode();
    if (did_pull) {
      RequestAsync(out);
      return {ErrorCode::kUnknown, {}};
    } else {
      return RequestSync(out);
    }
  }

 private:
  void RequestAsync(const std::vector<uint8_t> &out) {
    socket_.async_write_some(
        asio::buffer(out),
        [this, that = shared_from_this()](const auto &err, auto) {
          if (err) {
            did_pull_({ErrorCode::kUnknown, {}});
            return;
          }
          ReadMessageAsync();
        });
  }

  Pullable::PullResult RequestSync(const std::vector<uint8_t> &out) {
    Pullable::PullResult result{ErrorCode::kUnknown, {}};
    auto sent = socket_.write_some(asio::buffer(out));
    if (sent != out.size()) {
      return result;
    }
    auto message = ReadMessage();
    return message ? Pullable::PullResult{ErrorCode::kOK, message->Data()}
                   : result;
  }

  std::optional<Message> ReadMessage() {
    while (true) {
      auto bytes_transferred =
          socket_.read_some(asio::buffer(buffer_, buffer_.size()));
      if (bytes_transferred == 0) {
        return std::nullopt;
      }
      for (size_t decoded = 0; decoded < bytes_transferred;) {
        decoded +=
            message_.Decode(&buffer_[decoded], bytes_transferred - decoded);
        if (message_.IsSatisfied()) {
          return std::optional<Message>(std::move(message_));
        }
      }
    }
  }

  void ReadMessageAsync() {
    auto that = shared_from_this();
    socket_.async_read_some(
        asio::buffer(buffer_, buffer_.size()),
        [this, that](auto &error, size_t bytes_transferred) {
          if (bytes_transferred == 0) {
            did_pull_({ErrorCode::kUnknown, {}});
          }
          for (size_t decoded = 0; decoded < bytes_transferred;) {
            decoded +=
                message_.Decode(&buffer_[decoded], bytes_transferred - decoded);
            if (message_.IsSatisfied()) {
              did_pull_(Pullable::PullResult{ErrorCode::kOK, message_.Data()});
            } else {
              ReadMessageAsync();
            }
          }
        });
  }

 private:
  static const size_t kMaxBufferSize = 1024;

  asio::ip::tcp::socket socket_;
  std::vector<uint8_t> buffer_ = std::vector<uint8_t>(kMaxBufferSize, 0);
  Message message_;
  Pullable::DidPullHandler did_pull_;
};
}  // namespace gossip
#endif  // NODE_KEEPER_SRC_GOSSIP_PULL_SESSION_H_
