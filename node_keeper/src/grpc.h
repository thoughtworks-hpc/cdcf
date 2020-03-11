/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_GRPC_H_
#define NODE_KEEPER_SRC_GRPC_H_
#include <list>
#include <mutex>
#include <set>
#include <vector>

#include "src/channel.h"
#include "src/event.h"
#include "src/node_keeper.grpc.pb.h"

namespace node_keeper {
class GRPCImpl final : public NodeKeeper::Service {
 public:
  virtual ::grpc::Status GetMembers(::grpc::ServerContext* context,
                                    const ::google::protobuf::Empty* request,
                                    ::GetMembersReply* response);
  virtual ::grpc::Status Subscribe(::grpc::ServerContext* context,
                                   const ::SubscribeRequest* request,
                                   ::grpc::ServerWriter<::Event>* writer);

 public:
  void Notify(const std::vector<MemberEvent>& events) {
    for (auto& event : events) {
      switch (event.type) {
        case MemberEvent::kMemberUp:
          members_.insert(event.member);
          break;
        case MemberEvent::kMemberDown:
          members_.erase(event.member);
          break;
      }
    }
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& channel : channels_)
      for (auto& event : events) channel.Put(event);
  }

 private:
  Channel<MemberEvent>& AddChannel() {
    std::lock_guard<std::mutex> lock(mutex_);
    return channels_.emplace_back();
  }

  std::mutex mutex_;
  std::list<Channel<MemberEvent>> channels_;
  std::set<membership::Member> members_;
};
}  // namespace node_keeper
#endif  // NODE_KEEPER_SRC_GRPC_H_
