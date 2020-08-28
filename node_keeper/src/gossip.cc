/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/gossip.h"

#include <future>
#include <memory>

#include <asio.hpp>

#include "src/gossip/connection.h"
#include "src/gossip/message.h"
#include "src/gossip/pull_session.h"

namespace gossip {

using asio::ip::tcp, asio::ip::udp;

Transport::Transport(const Address &udp, const Address &tcp) try
    : upd_socket_(io_context_, udp::endpoint(udp::v4(), udp.port)),
      acceptor_(io_context_, tcp::endpoint(tcp::v4(), tcp.port)) {
} catch (const std::exception &e) {
  throw PortOccupied(e);
}

void Transport::Run() {
  StartReceiveGossip();
  StartAccept();
  io_thread_ = std::thread(&Transport::IORoutine, this);
}

Transport::~Transport() {
  CancelAllSessions();
  io_context_.stop();
  if (io_thread_.joinable()) {
    io_thread_.join();
  }
}

void Transport::CancelAllSessions() {
  decltype(pull_sessions_) pending_sessions;
  {
    std::lock_guard lock(mutex_);
    pending_sessions = pull_sessions_;
  }
  for (auto ref : pending_sessions) {
    if (auto session = ref.lock()) {
      session->Cancel();
    }
  }
}

ErrorCode Transport::Gossip(const std::vector<Address> &nodes,
                            const Payload &payload,
                            DidGossipHandler did_gossip) try {
  udp::resolver resolver(io_context_);
  for (const auto &node : nodes) {
    auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
    const auto &endpoint = *endpoints.begin();
    if (did_gossip) {
      upd_socket_.async_send_to(
          asio::buffer(payload.data), endpoint,
          [did_gossip](std::error_code error, std::size_t) {
            did_gossip(error ? ErrorCode::kUnknown : ErrorCode::kOK);
          });
    } else {
      auto sent = upd_socket_.send_to(asio::buffer(payload.data), endpoint);
      assert(sent == payload.data.size() && "all bytes should be sent");
    }
  }
  return ErrorCode::kOK;
} catch (const asio::system_error &e) {
  auto error = ExtractError(e);
  if (did_gossip) {
    did_gossip(error);
    return ErrorCode::kOK;
  } else {
    return error;
  }
}

void Transport::RegisterGossipHandler(GossipHandler handler) {
  gossip_handler_ = handler;
}

ErrorCode Transport::Push(const Address &node, const void *data, size_t size,
                          DidPushHandler did_push) try {
  tcp::resolver resolver(io_context_);
  auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
  tcp::socket socket(io_context_);
  asio::connect(socket, endpoints);
  auto buffer = Message(Message::Type::kPush, data, size).Encode();
  if (did_push) {
    socket.async_write_some(asio::buffer(buffer),
                            [did_push](const std::error_code &error, size_t) {
                              did_push(ErrorCode::kOK);
                            });
    return ErrorCode::kOK;
  } else {
    auto sent = socket.write_some(asio::buffer(buffer));
    return sent == buffer.size() ? ErrorCode::kOK : ErrorCode::kUnknown;
  }
} catch (const asio::system_error &e) {
  auto result = ExtractError(e);
  if (did_push) {
    asio::post(io_context_, std::bind(did_push, result));
    return ErrorCode::kOK;
  } else {
    return result;
  }
}

void Transport::RegisterPushHandler(PushHandler handler) {
  push_handler_ = handler;
}

Pullable::PullResult Transport::Pull(const Address &node, const void *data,
                                     size_t size, DidPullHandler did_pull) try {
  auto port = std::to_string(node.port);
  if (!did_pull) {
    PullSession session(&io_context_, node.host, port);
    return session.Request(data, size);
  }

  auto session = std::make_shared<PullSession>(&io_context_, node.host, port);
  auto it = pull_sessions_.end();
  {
    std::lock_guard lock(mutex_);
    it = pull_sessions_.insert(pull_sessions_.end(), session);
  }

  std::shared_ptr<int> guard(nullptr, [=](int *) {
    std::lock_guard lock(mutex_);
    pull_sessions_.erase(it);
  });
  return session->Request(data, size, [did_pull, it, this, guard](auto result) {
    return did_pull(result);
  });
} catch (const asio::system_error &e) {
  PullResult result{ExtractError(e), {}};
  if (did_pull) {
    asio::post(io_context_, std::bind(did_pull, result));
    return {ErrorCode::kOK, {}};
  }
  return result;
}

void Transport::RegisterPullHandler(PullHandler handler) {
  pull_handler_ = handler;
}

void Transport::StartReceiveGossip() {
  auto buffer =
      std::make_shared<std::vector<uint8_t>>(Payload::kMaxPayloadSize);
  auto remote = std::make_shared<udp::endpoint>();
  upd_socket_.async_receive_from(
      asio::buffer(*buffer), *remote,
      [this, buffer, remote](const std::error_code &error,
                             std::size_t bytes_transferred) {
        if (error) {
          return;
        }
        if (gossip_handler_) {
          const Address address{remote->address().to_string(), remote->port()};
          gossip_handler_(address, Payload(buffer->data(), bytes_transferred));
        }
        StartReceiveGossip();
      });
}

void Transport::IORoutine() {
  try {
    io_context_.run();
  } catch (...) {
  }
}

void Transport::StartAccept() {
  auto on_receive = [this](tcp::socket *socket, const Address &address,
                           const Message &message) {
    const auto &buffer = message.Data();
    if (message.Type() == Message::Type::kPush) {
      push_handler_(address, buffer.data(), buffer.size());
    } else if (message.Type() == Message::Type::kPull) {
      OnPull(socket, address, buffer);
    }
  };
  acceptor_.async_accept(
      [this, on_receive](const std::error_code &error, tcp::socket socket) {
        if (!error) {
          std::make_shared<Connection>(std::move(socket), on_receive)->Start();
        }
        StartAccept();
      });
}

void Transport::OnPull(tcp::socket *socket, const Address &address,
                       const std::vector<uint8_t> &request) {
  auto response = pull_handler_(address, request.data(), request.size());
  Message respondMessage(Message::Type::kPullResponse, response.data(),
                         response.size());
  auto out = respondMessage.Encode();
  socket->write_some(asio::buffer(out));
}

ErrorCode Transport::ExtractError(const asio::system_error &e) {
  switch (e.code().value()) {
    case asio::error::netdb_errors::host_not_found:
    case asio::error::netdb_errors::host_not_found_try_again:
      return ErrorCode::kHostNotFound;
    default:
      return ErrorCode::kUnknown;
  }
}

std::unique_ptr<Transportable> CreateTransport(const Address &udp,
                                               const Address &tcp) {
  return std::make_unique<Transport>(udp, tcp);
}

}  // namespace gossip
