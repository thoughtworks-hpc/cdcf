/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_GRPC_H_
#define NODE_KEEPER_SRC_GRPC_H_
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "src/channel.h"
#include "src/event.h"
#include "src/node_keeper.grpc.pb.h"

namespace node_keeper {
class GRPCImpl final : public ::NodeKeeper::Service {
 public:
  GRPCImpl(membership::Membership& cluster_membership);
  virtual ~GRPCImpl() { Close(); }
  virtual ::grpc::Status GetMembers(::grpc::ServerContext* context,
                                    const ::google::protobuf::Empty* request,
                                    ::GetMembersReply* response);
  virtual ::grpc::Status Subscribe(::grpc::ServerContext* context,
                                   const ::SubscribeRequest* request,
                                   ::grpc::ServerWriter<::Event>* writer);

  virtual ::grpc::Status PushActorsUpInfo(::grpc::ServerContext* context,
                                          const ::ActorsUpInfo* request,
                                          ::google::protobuf::Empty* response);

 public:
  void Notify(const std::vector<MemberEvent>& events);
  void Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& channel : channels_) {
      channel.Close();
    }
  }

  size_t GetSubscribersCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    return channels_.size();
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
  // Todo: 这里应该包含actors的信息
  std::set<membership::Member> members_;
  membership::Membership& cluster_membership_;
};

class GRPCServer {
 public:
  GRPCServer(const std::string& address, std::vector<grpc::Service*> services) {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    for (auto service : services) {
      builder.RegisterService(service);
    }
    server_ = builder.BuildAndStart();
  }

  ~GRPCServer() { server_->Shutdown(); }

 private:
  std::unique_ptr<grpc::Server> server_;
};
}  // namespace node_keeper
#endif  // NODE_KEEPER_SRC_GRPC_H_
