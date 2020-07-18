/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./include/yanghui_with_priority.h"

#include "./include/yanghui_demo_calculator.h"

int WorkerPool::Init() {
  auto members = actor_system::cluster::Cluster::GetInstance()->GetMembers();
  actor_system::cluster::Cluster::GetInstance()->AddObserver(this);

  for (const auto& member : members) {
    if (member.hostname == host_ || member.host == host_) {
      continue;
    }
    auto ret = AddWorker(member.host);
    if (ret != 0) {
      return ret;
    }
  }
  return 0;
}

bool WorkerPool::IsEmpty() { return workers_.empty(); }

caf::strong_actor_ptr WorkerPool::GetWorker() {
  if (workers_.empty()) {
    return caf::strong_actor_ptr();
  }
  if (worker_index_ == workers_.size() - 1) {
    worker_index_ = 0;
    return workers_[workers_.size() - 1];
  }
  return workers_[worker_index_++];
}

int WorkerPool::AddWorker(const std::string& host) {
  auto node = system_.middleman().connect(host, worker_port_);
  if (!node) {
    std::cerr << "*** connect failed: " << to_string(node.error()) << std::endl;
    return 1;
  }
  auto type = "CalculatorWithPriority";  // type of the actor we wish to spawn
  auto args = caf::make_message();       // arguments to construct the actor
  auto tout = std::chrono::seconds(30);  // wait no longer than 30s
  auto worker1 =
      system_.middleman().remote_spawn<caf::actor>(*node, type, args, tout);
  if (!worker1) {
    std::cerr << "*** remote spawn failed: " << to_string(worker1.error())
              << std::endl;
    return 1;
  }
  //  auto worker2 =
  //      system_.middleman().remote_spawn<calculator>(*node, type, args, tout);
  //  if (!worker2) {
  //    std::cerr << "*** remote spawn failed: " << to_string(worker2.error())
  //              << std::endl;
  //    return 1;
  //  }
  //  auto worker3 =
  //      system_.middleman().remote_spawn<calculator>(*node, type, args, tout);
  //  if (!worker3) {
  //    std::cerr << "*** remote spawn failed: " << to_string(worker3.error())
  //              << std::endl;
  //    return 1;
  //  }

  workers_.push_back(caf::actor_cast<caf::strong_actor_ptr>(*worker1));
  //  workers_.push_back(caf::actor_cast<caf::strong_actor_ptr>(*worker2));
  //  workers_.push_back(caf::actor_cast<caf::strong_actor_ptr>(*worker3));
  return 0;
}

void WorkerPool::Update(const actor_system::cluster::Event& event) {
  if (event.member.hostname != host_) {
    if (event.member.status == event.member.ActorSystemUp) {
      //         std::this_thread::sleep_for(std::chrono::seconds(2));
      AddWorker(event.member.host);
      PrintClusterMembers();
    } else if (event.member.status == event.member.Down) {
      std::cout << "detect worker node down, host:" << event.member.host
                << " port:" << event.member.port << std::endl;
    } else if (event.member.status == event.member.ActorSystemDown) {
      std::cout << "detect worker actor system down, host:" << event.member.host
                << " port:" << event.member.port << std::endl;
    }
  }
}

void WorkerPool::PrintClusterMembers() {
  auto members = actor_system::cluster::Cluster::GetInstance()->GetMembers();
  std::cout << "Current Cluster Members:" << std::endl;
  for (int i = 0; i < members.size(); ++i) {
    auto& member = members[i];
    std::cout << "Member " << i + 1 << ": ";
    std::cout << "name: " << member.name << ", hostname: " << member.hostname
              << ", ip: " << member.host << ", port:" << member.port
              << std::endl;
  }
}

using start_atom = caf::atom_constant<caf::atom("start")>;
using end_atom = caf::atom_constant<caf::atom("end")>;

caf::behavior yanghui_with_priority(caf::stateful_actor<yanghui_state>* self,
                                    WorkerPool* worker_pool,
                                    bool is_high_priority) {
  return {
      [=](const std::vector<std::vector<int>>& triangle_data) {
        self->state.triangle_sender_ = self->current_sender();
        self->state.last_level_results_ =
            std::vector<int>(triangle_data.size(), 0);
        self->state.last_level_results_[0] = triangle_data[0][0];
        self->state.triangle_data_.clear();
        for (int k = 0; k < triangle_data.size(); k++) {
          self->state.triangle_data_.emplace_back(triangle_data[k]);
        }
        self->send(self, start_atom::value);
      },
      [=](start_atom) {
        int i = self->state.level_;
        for (int j = 0; j < i + 1; j++) {
          auto worker = caf::actor_cast<caf::actor>(worker_pool->GetWorker());
          if (j == 0) {
            if (is_high_priority) {
              self->send<caf::message_priority::high>(
                  worker, "high priority", self->state.last_level_results_[0],
                  self->state.triangle_data_[i][j], j);
            } else {
              self->send(worker, self->state.last_level_results_[0],
                         self->state.triangle_data_[i][j], j);
            }
          } else if (j == i) {
            if (is_high_priority) {
              self->send<caf::message_priority::high>(
                  worker, "high priority",
                  self->state.last_level_results_[j - 1],
                  self->state.triangle_data_[i][j], j);
            } else {
              self->send(worker, self->state.last_level_results_[j - 1],
                         self->state.triangle_data_[i][j], j);
            }
          } else {
            if (is_high_priority) {
              self->send<caf::message_priority::high>(
                  worker, "high priority",
                  std::min(self->state.last_level_results_[j - 1],
                           self->state.last_level_results_[j]),
                  self->state.triangle_data_[i][j], j);
            } else {
              self->send<caf::message_priority::normal>(
                  worker,
                  std::min(self->state.last_level_results_[j - 1],
                           self->state.last_level_results_[j]),
                  self->state.triangle_data_[i][j], j);
            }
          }
        }
      },
      [=](std::string& result_with_position) {
        int index = result_with_position.find(":");
        int result = std::stoi(result_with_position.substr(0, index));
        int position = std::stoi(result_with_position.substr(
            index + 1, result_with_position.size() - index));

        caf::aout(self) << "level: " << self->state.level_
                        << " position: " << position
                        << " priority: " << is_high_priority << std::endl;
        self->state.last_level_results_[position] = result;
        self->state.count_++;
        if (self->state.level_ == self->state.count_ - 1) {
          if (self->state.level_ == self->state.triangle_data_.size() - 1) {
            self->state.level_ = 1;
            self->state.count_ = 0;
            self->send(self, end_atom::value);
          } else {
            if (is_high_priority) {
              caf::aout(self) << "finish level " << self->state.level_
                              << " with high priority" << std::endl;
            } else {
              caf::aout(self) << "finish level " << self->state.level_
                              << " with normal priority " << std::endl;
            }

            self->state.level_++;
            self->state.count_ = 0;
            self->send(self, start_atom::value);
          }
        }
      },
      [=](end_atom) {
        NumberCompareData send_data;
        send_data.numbers = self->state.last_level_results_;
        auto worker = caf::actor_cast<caf::actor>(worker_pool->GetWorker());
        self->request(worker, caf::infinite, "high priority", send_data)
            .await([=](int final_result) {
              if (is_high_priority) {
                self->send(
                    caf::actor_cast<caf::actor>(self->state.triangle_sender_),
                    is_high_priority, final_result);
              } else {
                self->send(
                    caf::actor_cast<caf::actor>(self->state.triangle_sender_),
                    is_high_priority, final_result);
              }
            });
      }};
}

caf::behavior yanghui_job_dispatcher(
    caf::stateful_actor<dispatcher_state>* self, caf::actor target1,
    caf::actor target2) {
  return {[=](const std::vector<std::vector<int>>& triangle_data) {
            self->state.triangle_sender_ = self->current_sender();
            self->send(target1, triangle_data);
            self->send(target2, triangle_data);
          },
          [=](bool is_high_priority, int result) {
            self->state.result.emplace_back(
                std::make_pair(is_high_priority, result));
            if (self->state.result.size() == 2) {
              self->send(
                  caf::actor_cast<caf::actor>(self->state.triangle_sender_),
                  self->state.result);
              self->state.result.clear();
            }
          }};
}

