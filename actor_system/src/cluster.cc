/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "cdcf/cluster/cluster.h"

#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include <thread>

#include "cdcf/logger.h"
#include "src/node_keeper.grpc.pb.h"

namespace cdcf::cluster {

class ClusterImpl {
 public:
  // Todo: duplicate code
  ClusterImpl() {
    const std::string& host{"127.0.0.1"};
    const uint16_t port = 50051;
    auto address = host + ":" + std::to_string(port);
    CDCF_LOGGER_INFO("ClusterImpl connect to node keeper address: {}", address);
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
    CDCF_LOGGER_INFO(
        "ClusterImpl connect to node keeper with parameter address: {}",
        address);
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
    CDCF_LOGGER_CRITICAL("ClusterImpl destruct start!~!!!!!!!!");
    if (thread_.joinable()) {
      CDCF_LOGGER_CRITICAL("ClusterImpl join the thread");
      thread_.join();
    }
    //    stub_.release();
    (void)stub_.release();
    if (stub_ == nullptr) {
      CDCF_LOGGER_CRITICAL("stub release");
    }
    CDCF_LOGGER_CRITICAL("ClusterImpl destruct finish!~!!!!!!!");
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
      CDCF_LOGGER_ERROR(
          "Send actor system up to self node keeper, error code: {}, error "
          "msg: {}, detail: {}",
          status.error_code(), status.error_message(), status.error_details());
    }
  }

 private:
  void Routine() {
    Fetch();
    grpc::ClientContext context;
    auto reader(stub_->Subscribe(&context, {}));
    CDCF_LOGGER_CRITICAL("Routine start");
    for (::Event event; !stop_ && reader->Read(&event);) {
      Update(event);
    }
    CDCF_LOGGER_CRITICAL("Routine finish");
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
  std::vector<std::string> ip_list_;
};

std::mutex Cluster::instance_mutex_;
Cluster* Cluster::instance_;

Cluster* Cluster::GetInstance() {
  if (instance_) {
    return instance_;
  }
  std::lock_guard lock(instance_mutex_);
  if (instance_) {
    return instance_;
  }
  instance_ = new Cluster();
  CDCF_LOGGER_CRITICAL("new instance!!");
  return instance_;
}

Cluster* Cluster::GetInstance(const std::string& host_ip, uint16_t port) {
  if (instance_) {
    return instance_;
  }
  std::lock_guard lock(instance_mutex_);
  if (instance_) {
    return instance_;
  }
  //  instance_.reset(new Cluster(host_ip, port));
  instance_ = new Cluster();
  return instance_;
}

void Cluster::DeleteInstance() { delete instance_; }

std::vector<Member> Cluster::GetMembers() { return impl_->GetMembers(); }

Cluster::Cluster() : impl_(std::make_unique<ClusterImpl>()) {}

Cluster::Cluster(const std::string& host_ip, uint16_t port)
    : impl_(std::make_unique<ClusterImpl>(host_ip, port)) {}

Cluster::~Cluster() {}

void Cluster::NotifyReady() { impl_->NotifyReady(); }

}  // namespace cdcf::cluster
