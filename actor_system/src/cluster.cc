/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "cdcf/actor_system/cluster.h"

#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include <thread>

#include "src/node_keeper.grpc.pb.h"

namespace actor_system {
namespace cluster {

class ClusterImpl {
 public:
  ClusterImpl() {
    const std::string& host{"127.0.0.1"};
    const uint16_t port = 50051;
    auto address = host + ":" + std::to_string(port);
    std::cout << "ClusterImpl connect to node keeper address:" << address
              << std::endl;
    auto channel =
        grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
    channel->WaitForConnected(deadline);
    stub_ = NodeKeeper::NewStub(channel);
    Fetch();
    thread_ = std::thread(&ClusterImpl::Routine, this);
  }

  ClusterImpl(const std::string& host_ip, uint16_t port) {
    const std::string& host{host_ip};
    auto address = host + ":" + std::to_string(port);
    std::cout << "ClusterImpl connect to node keeper with parameter address:"
              << address << std::endl;
    auto channel =
        grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
    channel->WaitForConnected(deadline);
    stub_ = NodeKeeper::NewStub(channel);
    Fetch();
    thread_ = std::thread(&ClusterImpl::Routine, this);
  }

  ~ClusterImpl() {
    stop_ = true;
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  std::vector<Member> GetMembers() {
    std::lock_guard lock(mutex_members_);
    return members_;
  }

  void NotifyReady() {
    grpc::ClientContext context;
    ::google::protobuf::Empty empty;
    auto status = stub_->ActorSystemUp(&context, {}, &empty);
    if (!status.ok()) {
      std::cout << "[ActorSystemUp] error code:  " << status.error_code()
                << ",msg: " << status.error_message()
                << ", detail: " << status.error_details() << std::endl;
    }
  }

 private:
  void Routine() {
    Fetch();
    grpc::ClientContext context;
    auto reader(stub_->Subscribe(&context, {}));
    for (::Event event; !stop_ && reader->Read(&event);) {
      Update(event);
    }
  }

  void Fetch() {
    ::GetMembersReply reply;
    grpc::ClientContext context;
    auto status = stub_->GetMembers(&context, {}, &reply);
    if (status.ok()) {
      std::vector<Member> members;
      members.reserve(reply.members().size());
      for (auto member : reply.members()) {
        auto port = static_cast<uint16_t>(member.port());
        members.emplace_back(member.name(), member.hostname(), member.host(),
                             member.role(), port, Member::Status::Up);
      }
      {
        std::lock_guard lock(mutex_members_);
        std::swap(members_, members);
      }
    }
  }

  void Update(::Event& event) {
    if (event.type() != Event_Type_MEMBER_CHANGED) {
      return;
    }
    ::MemberEvent member_event;
    event.data().UnpackTo(&member_event);
    const auto& detail = member_event.member();
    auto port = static_cast<uint16_t>(detail.port());
    Member member{detail.name(), detail.hostname(), detail.host(),
                  detail.role(), port};
    if (member_event.status() == ::MemberEvent::UP) {
      member.status = Member::Status::Up;
      std::lock_guard lock(mutex_members_);
      members_.push_back(member);
    } else if (member_event.status() == ::MemberEvent::DOWN) {
      member.status = Member::Status::Down;
      {
        std::lock_guard lock(mutex_members_);
        auto it = std::remove(members_.begin(), members_.end(), member);
        members_.erase(it, members_.end());
      }
    } else if (member_event.status() == ::MemberEvent::ACTOR_SYSTEM_DOWN) {
      member.status = Member::Status::ActorSystemDown;
    } else if (member_event.status() == ::MemberEvent::ACTOR_SYSTEM_UP) {
      std::cout << "*** cluster receive actor system up" << std::endl;
      member.status = Member::Status::ActorSystemUp;
    }
    Cluster::GetInstance()->Notify({member});
  }

  std::mutex mutex_members_;
  std::vector<Member> members_;
  std::unique_ptr<NodeKeeper::Stub> stub_;
  std::thread thread_;
  bool stop_{false};
};

std::mutex Cluster::instance_mutex_;
std::unique_ptr<Cluster> Cluster::instance_;

Cluster* Cluster::GetInstance() {
  if (instance_) {
    return &*instance_;
  }
  std::lock_guard lock(instance_mutex_);
  if (instance_) {
    return &*instance_;
  }
  instance_.reset(new Cluster());
  return &*instance_;
}

Cluster* Cluster::GetInstance(const std::string& host_ip, uint16_t port) {
  if (instance_) {
    return &*instance_;
  }
  std::lock_guard lock(instance_mutex_);
  if (instance_) {
    return &*instance_;
  }
  instance_.reset(new Cluster(host_ip, port));
  return &*instance_;
}

std::vector<Member> Cluster::GetMembers() { return impl_->GetMembers(); }

Cluster::Cluster() : impl_(std::make_unique<ClusterImpl>()) {}

Cluster::Cluster(const std::string& host_ip, uint16_t port)
    : impl_(std::make_unique<ClusterImpl>(host_ip, port)) {}

Cluster::~Cluster() {}

void Cluster::NotifyReady() { impl_->NotifyReady(); }

}  // namespace cluster
};  // namespace actor_system
