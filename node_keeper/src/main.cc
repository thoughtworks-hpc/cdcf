/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "../include/membership.h"
#include "./src/config.h"

int main(int argc, char *argv[]) {
  node_keeper::Config config;
  auto ret = config.parse_config(argc, argv, "cdcf-default.ini");
  if (ret != cdcf_config::SUCCESS) {
    return 1;
  }

  membership::Config member_config;
  member_config.AddHostMember("node1", config.host_, config.port);
  gossip::Address address{config.host_, config.port_};
  auto transport = gossip::CreateTransport(address, address);
  membership::Membership membership;
  membership.Init(transport, config);

  std::vector<membership::Member> members = membership.GetMembers();
  EXPECT_EQ(members.size(), 1);
  EXPECT_EQ(members[0], member);
  std::cout << "host: " << config.host_ << std::endl;
  std::cout << "port: " << config.port_ << std::endl;
  return 0;
}
