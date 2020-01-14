//
// Created by Zhao Wenbo on 2020/1/14.
//

#ifndef CDCF_MEMBERSHIP_H
#define CDCF_MEMBERSHIP_H

#include <string>

namespace membership {

class Member {
 private:
  std::string node_name_;
  std::string ip_address_;
  short port_;

 public:
  Member(const std::string& node_name, const std::string& ip_address,
         short port_)
      : node_name_(node_name), ip_address_(ip_address), port_(port_) {}

  const std::string& GetNodeName() const { return node_name_; }
  const std::string& GetIpAddress() const { return ip_address_; }
  short GetPort() const { return port_; }
};

};  // namespace membership

#endif  // CDCF_MEMBERSHIP_H
