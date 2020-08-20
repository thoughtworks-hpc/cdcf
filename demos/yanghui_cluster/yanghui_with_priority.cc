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

bool WorkerPool::IsEmpty() const {
  std::shared_lock lock(workers_mutex_);
  return workers_.empty();
}

caf::strong_actor_ptr WorkerPool::GetWorker() {
  std::shared_lock lock(workers_mutex_);
  if (workers_.empty()) {
    return caf::strong_actor_ptr();
  }
  if (worker_index_ == workers_.size() - 1) {
    worker_index_ = 0;
    return workers_[workers_.size() - 1];
  }
  auto worker_index_to_return = worker_index_.load();
  worker_index_++;
  return workers_[worker_index_to_return];
}

int WorkerPool::AddWorker(const std::string& host) {
  auto actor = yanghui_io_.remote_actor(system_, host, worker_port_);
  if (!actor) {
    CDCF_LOGGER_ERROR("connect failed. host: {}, port: {}, error: {}", host,
                      worker_port_, system_.render(actor.error()));
    return 1;
  }
  auto type = "CalculatorWithPriority";  // type of the actor we wish to spawn
  auto args = caf::make_message();       // arguments to construct the actor
  auto tout = std::chrono::seconds(30);  // wait no longer than 30s
  auto worker1 = system_.middleman().remote_spawn<caf::actor>(actor->node(),
                                                              type, args, tout);
  if (!worker1) {
    CDCF_LOGGER_ERROR("remote spawn failed. error: {}",
                      system_.render(worker1.error()));
    return 1;
  }
  std::cout << "add worker for calculator with priority on host: " << host
            << std::endl;
  CDCF_LOGGER_DEBUG("add worker for calculator with priority on host: {}",
                    host);

  std::unique_lock lock(workers_mutex_);
  workers_.push_back(caf::actor_cast<caf::strong_actor_ptr>(*worker1));

  return 0;
}

void WorkerPool::Update(const actor_system::cluster::Event& event) {
  if (event.member.hostname != host_) {
    if (event.member.status == event.member.ActorSystemUp) {
      AddWorker(event.member.host);
      //      PrintClusterMembers();
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
              self->send(worker, cdcf::actor_system::high_priority_atom::value,
                         self->state.last_level_results_[0],
                         self->state.triangle_data_[i][j], j);
            } else {
              self->send(worker, self->state.last_level_results_[0],
                         self->state.triangle_data_[i][j], j);
            }
          } else if (j == i) {
            if (is_high_priority) {
              self->send(worker, cdcf::actor_system::high_priority_atom::value,
                         self->state.last_level_results_[j - 1],
                         self->state.triangle_data_[i][j], j);
            } else {
              self->send(worker, self->state.last_level_results_[j - 1],
                         self->state.triangle_data_[i][j], j);
            }
          } else {
            if (is_high_priority) {
              self->send(worker, cdcf::actor_system::high_priority_atom::value,
                         std::min(self->state.last_level_results_[j - 1],
                                  self->state.last_level_results_[j]),
                         self->state.triangle_data_[i][j], j);
            } else {
              self->send(worker,
                         std::min(self->state.last_level_results_[j - 1],
                                  self->state.last_level_results_[j]),
                         self->state.triangle_data_[i][j], j);
            }
          }
        }
      },
      [=](ResultWithPosition& result_with_position) {
        std::string priority_text;
        if (is_high_priority) {
          priority_text = "high";
        } else {
          priority_text = "normal";
        }
        caf::aout(self) << "received result for level " << self->state.level_
                        << " at position " << result_with_position.position
                        << " with priority: " << priority_text << std::endl;
        self->state.last_level_results_[result_with_position.position] =
            result_with_position.result;
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
        self->request(worker, caf::infinite, cdcf::actor_system::high_priority_atom::value,
                      send_data)
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
            auto time_now = std::chrono::steady_clock::now();
            if (is_high_priority) {
              caf::aout(self)
                  << "high priority job finished at: " << time_now << std::endl;
            } else {
              caf::aout(self) << "normal priority job finished at: " << time_now
                              << std::endl;
            }
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
