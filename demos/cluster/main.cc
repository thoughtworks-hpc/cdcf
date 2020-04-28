/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include <actor_system.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include <asio.hpp>

#include "./config.h"

using asio::ip::tcp;

class WorkerRouter : public actor_system::cluster::Observer {
 public:
  WorkerRouter(caf::actor_system& system, const std::string host, uint16_t port)
      : system_(system), host_(host), port_(port) {
    FetchMembers();
    actor_system::cluster::Cluster::GetInstance()->AddObserver(this);
  }
  ~WorkerRouter() {
    actor_system::cluster::Cluster::GetInstance()->RemoveObserver(this);
  }

  std::function<std::string(const std::string&)> Route() {
    FetchMembers();
    const auto members = members_;
    std::vector<std::string> result(members.size());
    std::transform(members.begin(), members.end(), result.begin(),
                   [](const auto& member) {
                     return member.host + ":" + std::to_string(member.port);
                   });
    std::sort(result.begin(), result.end());
    std::stringstream ss;
    std::copy(result.begin(), result.end(),
              std::ostream_iterator<std::string>(ss, "\n"));
    return [result = ss.str()](const std::string&) {
      return result.empty() ? "\n" : result;
    };
  }

  void Update(const actor_system::cluster::Event&) override {
    // FIXME: handle event here instead of fetch
    FetchMembers();
  }

 private:
  void FetchMembers() {
    auto cluster = actor_system::cluster::Cluster::GetInstance();
    members_ = cluster->GetMembers();
    std::cout << "[worker] all members: " << cluster->GetMembers().size()
              << std::endl;
  }

  caf::actor_system& system_;
  std::vector<actor_system::cluster::Member> members_;
  std::string host_;
  uint16_t port_;
};

static std::unique_ptr<WorkerRouter> router;

class Session : public std::enable_shared_from_this<Session> {
 public:
  explicit Session(tcp::socket socket) : socket_(std::move(socket)) {}

  void Start() { DoRead(); }

 private:
  void DoRead() {
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(data_, data_.size()),
                            [this, self](std::error_code ec, size_t length) {
                              if (!ec) {
                                auto result = router->Route()(
                                    std::string(&data_[0], &data_[length]));
                                DoWrite(result);
                              }
                            });
  }

  void DoWrite(const std::string& result) {
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(result.c_str(), result.size()),
                      [this, self](std::error_code ec, std::size_t /*length*/) {
                        if (!ec) {
                          DoRead();
                        }
                      });
  }

  tcp::socket socket_;
  std::array<char, 1024> data_;
};

class Server {
 public:
  Server(asio::io_context& io_context, uint16_t port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    DoAccept();
  }

 private:
  void DoAccept() {
    acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
      if (!ec) {
        std::make_shared<Session>(std::move(socket))->Start();
      }
      DoAccept();
    });
  }

  tcp::acceptor acceptor_;
};

void caf_main(caf::actor_system& system, const Config& config) {
  std::cout << "this node host: " << config.host_ << std::endl;
  auto local_port = config.port_;
  auto remote_port = config.port_;
  router = std::make_unique<WorkerRouter>(system, config.host_, remote_port);

  try {
    asio::io_context io_context;
    Server s(io_context, config.reception_port_);
    io_context.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}

CAF_MAIN(caf::io::middleman)
