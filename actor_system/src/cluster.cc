/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "include/actor_system/cluster.h"

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
    std::lock_guard lock(mutex_);
    return members_;
  }

  void PushActorsUpToNodeKeeper(std::vector<caf::actor> up_actors) {
    ActorsUpInfo request;

    for (int i = 0; i < up_actors.size(); i++) {
      auto address = caf::to_string(up_actors[i].address());
      request.add_addresses(address);
    }

    grpc::ClientContext context;
    ::GetMembersReply reply;
    ::google::protobuf::Empty empty;

    auto status = stub_->PushActorsUpInfo(&context, request, &empty);
    if (status.ok()) {
      std::cout << "[PushActorsUpToNodeKeeper] rpc ok" << std::endl;
    } else {
      // Todo(Yujia.Li): 记日志
      std::cout << "[PushActorsUpToNodeKeeper] error code:  "
                << status.error_code() << ",msg: " << status.error_message()
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
                             port, Member::Status::Up);
      }
      {
        std::lock_guard lock(mutex_);
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
    Member member{detail.name(), detail.hostname(), detail.host(), port};
    if (member_event.status() == ::MemberEvent::UP) {
      member.status = Member::Status::Up;
      std::lock_guard lock(mutex_);
      members_.push_back(member);
    } else if (member_event.status() == ::MemberEvent::DOWN) {
      member.status = Member::Status::Down;
      std::lock_guard lock(mutex_);
      auto it = std::remove(members_.begin(), members_.end(), member);
      members_.erase(it, members_.end());
    } else if (member_event.status() == ::MemberEvent::ACTORS_UP) {
      auto actors = member_event.actors().addresses();

      std::cout << "[actor system] receive actors up, size: " << actors.size()
                << std::endl;
      for (auto& actor_address : actors) {
        std::cout << "[actor system] " << actor_address << " up." << std::endl;
      }
    }
    Cluster::GetInstance()->Notify({member});
  }

  std::mutex mutex_;
  std::vector<Member> members_;
  // Todo(Yujia.Li) 在这里添加ActorMonitor
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

std::vector<Member> Cluster::GetMembers() { return impl_->GetMembers(); }

Cluster::Cluster() : impl_(std::make_unique<ClusterImpl>()) {}

Cluster::~Cluster() {}

void Cluster::PushActorsUpToNodeKeeper(std::vector<caf::actor> up_actors) {
  impl_->PushActorsUpToNodeKeeper(up_actors);
}

}  // namespace cluster
};  // namespace actor_system
