/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include <cdcf/actor_system.h>

#include <array>
#include <iostream>
#include <memory>
#include <utility>

#include <asio.hpp>

#include "./config.h"
#include "./worker.h"

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

  caf::function_view<Hello> Route(const std::string& name) {
    auto member = members_[rand() % members_.size()];
    std::cout << "routee: " << member.host << ":" << port_ << std::endl;
    auto node = system_.middleman().connect(member.host, port_);
    if (!node) {
      std::cerr << "*** connect failed: " << system_.render(node.error())
                << std::endl;
      throw std::runtime_error("failed to connect cluster");
    }
    auto args = caf::make_message();
    auto timeout = std::chrono::seconds(30);
    auto worker =
        system_.middleman().remote_spawn<Hello>(*node, name, args, timeout);
    if (!worker) {
      std::cerr << "*** remote spawn failed: " << system_.render(worker.error())
                << std::endl;
      throw std::runtime_error("failed to spawn actor");
    }
    return make_function_view(*worker);
  }

  void Update(const actor_system::cluster::Event&) override {
    // FIXME: handle event here instead of fetch
    FetchMembers();
  }

 private:
  void FetchMembers() {
    auto cluster = actor_system::cluster::Cluster::GetInstance();
    members_ = cluster->GetMembers();
    std::cout << "all members: " << cluster->GetMembers().size() << std::endl;
    auto it = std::remove_if(members_.begin(), members_.end(),
                             [&](auto& m) { return m.host == host_; });
    members_.erase(it, members_.end());
    std::cout << "filtered members: " << members_.size() << std::endl;
  }

  caf::actor_system& system_;
  std::vector<actor_system::cluster::Member> members_;
  std::string host_;
  uint16_t port_;
};

static std::unique_ptr<WorkerRouter> router;

void InitWorker(caf::actor_system& system, uint16_t port) {
  std::cout << "actor system port: " << port << std::endl;
  auto res = system.middleman().open(port);
  if (!res) {
    std::cerr << "*** cannot open port: " << system.render(res.error())
              << std::endl;
    return;
  }
}

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
                                auto result = router->Route("Hello")(
                                    std::string(&data_[0], &data_[length]));
                                // anon_send_exit(*worker, exit_reason::kill);
                                DoWrite(*result);
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

std::string the_host_name;  // NOLINT

void caf_main(caf::actor_system& system, const Config& config) {
  the_host_name = config.host_;
  std::cout << "this node host: " << config.host_ << std::endl;
  auto local_port = config.port_;
  auto remote_port = config.port_;
  router = std::make_unique<WorkerRouter>(system, config.host_, remote_port);
  InitWorker(system, local_port);

  try {
    asio::io_context io_context;
    Server s(io_context, config.reception_port_);
    io_context.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}

CAF_MAIN(caf::io::middleman)
