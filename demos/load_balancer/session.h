/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef DEMOS_LOAD_BALANCER_SESSION_H_
#define DEMOS_LOAD_BALANCER_SESSION_H_

#include <string>
#include <utility>

class Session : public std::enable_shared_from_this<Session> {
 public:
  explicit Session(asio::ip::tcp::socket socket, WorkerRouter& router)
      : socket_(std::move(socket)), router_(router) {}

  void Start() { DoRead(); }

 private:
  std::string HandleReport() const {
    std::stringstream ss;
    for (auto stat : router_.Stats()) {
      ss << "==========\n"
         << "name: " << stat.name << "\n"
         << "count: " << stat.count << std::endl;
    }
    return ss.str();
  }

  std::string HandleStart(std::istream& in) const {
    size_t count = 0;
    size_t x = 3'300'000;
    in >> count >> x;
    router_.Start(count, x);
    return std::to_string(count) + " tasks of factorial(" + std::to_string(x) +
           ") were sent\n";
  }

  std::string HandleRequest(const std::string& request) {
    std::stringstream ss(request);
    std::string command;
    ss >> command;
    if (command == "report") {
      return HandleReport();
    } else if (command == "start") {
      return HandleStart(ss);
    } else {
      return "type `start` to clear stats and start over\n"
             "type `report` to report work stats\n";
    }
  }

  void DoRead() {
    auto self(shared_from_this());
    socket_.async_read_some(
        asio::buffer(data_, data_.size()),
        [this, self](std::error_code ec, size_t length) {
          if (!ec) {
            auto result = HandleRequest(std::string(&data_[0], &data_[length]));
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

  asio::ip::tcp::socket socket_;
  std::array<char, 1024> data_;
  WorkerRouter& router_;
};
#endif  // DEMOS_LOAD_BALANCER_SESSION_H_
