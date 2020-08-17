/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef DEMOS_LOAD_BALANCER_ROUTER_H_
#define DEMOS_LOAD_BALANCER_ROUTER_H_
#include <cdcf/actor_system.h>

#include <mutex>
#include <string>
#include <utility>
#include <vector>

struct WorkerStats {
  std::string name;
  size_t count;
};

class WorkerRouter : public actor_system::cluster::Observer {
 public:
  WorkerRouter(caf::actor_system& system, const std::string host, uint16_t port)
      : system_(system), host_(host), port_(port) {
    InitLoadBalancer();
  }

  ~WorkerRouter() {
    actor_system::cluster::Cluster::GetInstance()->RemoveObserver(this);
  }

  void AddWorker(const actor_system::cluster::Member& member) {
    caf::actor worker;
    for (;;) {
      try {
        worker = SpawnWorkers(member);
        break;
      } catch (const std::exception& e) {
        std::cout << "exception: " << e.what() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }

    caf::anon_send(balancer_, caf::sys_atom::value, caf::put_atom::value,
                   worker);

    members_.emplace_back(member);
    workers_.emplace_back(worker);
  }

  void DeleteWorker(const actor_system::cluster::Member& member) {
    auto it = std::find_if(members_.begin(), members_.end(), [&](auto& m) {
      return m.host == member.host && m.port == member.port;
    });
    if (it == members_.end()) {
      return;
    }
    auto index = it - members_.begin();
    auto worker = workers_[index];
    caf::anon_send(balancer_, caf::sys_atom::value, caf::delete_atom::value,
                   worker);
    members_.erase(it);
    workers_.erase(workers_.begin() + index);
  }

  void Update(const actor_system::cluster::Event& event) override {
    if (event.member.status == actor_system::cluster::Member::Up) {
      std::lock_guard lock{mutex_};
      AddWorker(event.member);
    } else if (event.member.status == actor_system::cluster::Member::Down) {
      std::lock_guard lock{mutex_};
      DeleteWorker(event.member);
    }
  }

  void Start(size_t count, size_t x = size_t{3'300'000}) {
    for (size_t i = 0; i < count; ++i) {
      caf::anon_send(balancer_, x);
    }
  }

  auto Stats() const {
    std::vector<WorkerStats> result(members_.size());
    std::generate_n(result.begin(), members_.size(), [&, i = 0]() mutable {
      const auto& name = members_[i].name;
      const auto message = make_function_view(workers_[i])(0);
      ++i;
      return WorkerStats{name, message->get_as<size_t>(0)};
    });
    return result;
  }

 private:
  void InitLoadBalancer() {
    auto policy = cdcf::load_balancer::policy::MinLoad();
    balancer_ = cdcf::load_balancer::Router::Make(&context_, std::move(policy));
    actor_system::cluster::Cluster::GetInstance()->AddObserver(this);
    std::lock_guard lock{mutex_};
    for (const auto& member : FetchMembers()) {
      AddWorker(member);
    }
  }

  std::vector<actor_system::cluster::Member> FetchMembers() const {
    auto cluster = actor_system::cluster::Cluster::GetInstance();
    auto members = cluster->GetMembers();
    std::cout << "all members: " << cluster->GetMembers().size() << std::endl;
    auto it = std::remove_if(members.begin(), members.end(),
                             [&](auto& m) { return m.name == "server"; });
    members.erase(it, members.end());
    std::cout << "filtered members: " << members.size() << std::endl;
    return members;
  }

  caf::actor SpawnWorkers(const actor_system::cluster::Member& member) const {
    std::cout << "routee: " << member.host << ":" << port_ << std::endl;
    auto node = system_.middleman().connect(member.host, port_);
    if (!node) {
      std::cerr << "*** connect failed: " << system_.render(node.error())
                << std::endl;
      throw std::runtime_error("failed to connect cluster");
    }
    const auto type = "Calculator";
    auto args = caf::make_message();
    auto timeout = std::chrono::seconds(30);
    auto worker = system_.middleman().remote_spawn<caf::actor>(*node, type,
                                                               args, timeout);
    if (!worker) {
      std::cerr << "*** remote spawn failed: " << system_.render(worker.error())
                << std::endl;
      throw std::runtime_error("failed to spawn actor");
    }
    return *worker;
  }

  caf::actor_system& system_;
  caf::scoped_execution_unit context_{&system_};
  std::vector<actor_system::cluster::Member> members_;
  std::string host_;
  uint16_t port_;
  std::mutex mutex_;
  std::vector<caf::actor> workers_;
  caf::actor balancer_;
};

#endif  // DEMOS_LOAD_BALANCER_ROUTER_H_
