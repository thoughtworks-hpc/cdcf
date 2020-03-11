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
  virtual ~GRPCImpl() { Close(); }
  virtual ::grpc::Status GetMembers(::grpc::ServerContext* context,
                                    const ::google::protobuf::Empty* request,
                                    ::GetMembersReply* response);
  virtual ::grpc::Status Subscribe(::grpc::ServerContext* context,
                                   const ::SubscribeRequest* request,
                                   ::grpc::ServerWriter<::Event>* writer);

 public:
  void Notify(const std::vector<MemberEvent>& events);
  void Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& channel : channels_) {
      channel.Close();
    }
  }

 private:
  using channels_type = std::list<Channel<MemberEvent>>;

  channels_type::iterator AddChannel() {
    std::lock_guard<std::mutex> lock(mutex_);
    channels_.emplace_back();
    return std::prev(channels_.end());
  }

  void RemoveChannel(channels_type::iterator it) {
    std::lock_guard<std::mutex> lock(mutex_);
    channels_.erase(it);
  }

  std::mutex mutex_;
  channels_type channels_;
  std::set<membership::Member> members_;
};
}  // namespace node_keeper
#endif  // NODE_KEEPER_SRC_GRPC_H_
