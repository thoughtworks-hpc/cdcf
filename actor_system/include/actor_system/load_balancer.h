/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_LOAD_BALANCER_H_
#define ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_LOAD_BALANCER_H_

#include <limits>
#include <unordered_map>

#include <caf/all.hpp>

namespace cdcf {
class load_balancer : public caf::monitorable_actor {
 public:
  static caf::actor make(caf::execution_unit *host) {
    auto &sys = host->system();
    caf::actor_config config{host};
    auto res = caf::make_actor<load_balancer, caf::actor>(
        sys.next_actor_id(), sys.node(), &sys, config);
    auto ptr = static_cast<load_balancer *>(
        caf::actor_cast<caf::abstract_actor *>(res));
    return res;
  }

 public:
  explicit load_balancer(caf::actor_config &config)
      : caf::monitorable_actor(config),
        planned_reason_(caf::exit_reason::normal) {}

  void enqueue(caf::mailbox_element_ptr what,
               caf::execution_unit *host) override {
    caf::upgrade_lock<caf::detail::shared_spinlock> guard{workers_mtx_};
    if (Filter(guard, what->sender, what->mid, *what, host)) {
      return;
    }
    std::cout << "what: " << what->mid.integer_value() << ": "
              << what->mid.category() << ", "
              << what->mid.request_id().integer_value() << ", "
              << what->mid.response_id().integer_value() << std::endl;
    if (what->sender) {
      std::cout << "sender: " << what->sender->aid << std::endl;
    }
    if (IsReply(what)) {
      return HandleReply(what, host, guard);
    } else {
      return Proxy(what, host, guard);
    }
  }

  void on_destroy() override {
    CAF_PUSH_AID_FROM_PTR(this);
    if (!getf(is_cleaned_up_flag)) {
      cleanup(caf::exit_reason::unreachable, nullptr);
      monitorable_actor::on_destroy();
      unregister_from_system();
    }
  }

 protected:
  void on_cleanup(const caf::error &reason) override {
    CAF_PUSH_AID_FROM_PTR(this);
    CAF_IGNORE_UNUSED(reason);
    CAF_LOG_TERMINATE_EVENT(this, reason);
  }

 private:
  void HandleReply(caf::mailbox_element_ptr &what, caf::execution_unit *host,
                   caf::upgrade_lock<caf::detail::shared_spinlock> &guard) {
    auto from = caf::actor_cast<caf::actor>(what->sender);
    --loads_[from];
    auto caller = FindCaller(what);
    if (!caller) {
      return;
    }
    guard.unlock();
    const auto msg = what->move_content_to_message();
    caller->enqueue(ctrl(), what->mid, msg, host);
    return;
  }

  bool TrackProxy(const caf::mailbox_element_ptr &what) {
    bool required_reply = what->mid != what->mid.response_id();
    if (required_reply) {
      // store original sender
      auto from = caf::actor_cast<caf::actor>(what->sender);
      callers_[what->mid.response_id()] = from;
    }
    return required_reply;
  }

  caf::actor FindCaller(const caf::mailbox_element_ptr &what) {
    auto it = callers_.find(what->mid);
    if (it == callers_.end()) {
      std::cout << "ignore reply from: " << what->sender->aid << std::endl;
      return nullptr;
    }
    callers_.erase(it);
    return it->second;
  }

  void Proxy(caf::mailbox_element_ptr &what, caf::execution_unit *host,
             caf::upgrade_lock<caf::detail::shared_spinlock> &guard) {
    std::cout << "forwarding ---> " << std::endl;
    auto actor = Policy(home_system(), workers_, what, host);
    ++loads_[actor];
    TrackProxy(what);
    guard.unlock();
    // replace sender with load_balancer
    actor->enqueue(ctrl(), what->mid, what->move_content_to_message(), host);
    return;
  }

  bool IsReply(caf::mailbox_element_ptr const &what) const {
    auto it = std::find(workers_.begin(), workers_.end(), what->sender);
    return it != workers_.end();
  }

  size_t GetWorkerLoad(const caf::actor &actor) {
    auto it = loads_.find(actor);
    return it == loads_.end() ? 0 : it->second;
  }

  caf::actor Policy(caf::actor_system &system,
                    const std::vector<caf::actor> &actors,
                    caf::mailbox_element_ptr &mail, caf::execution_unit *host) {
    CAF_ASSERT(!actors.empty());
    for (auto &[actor, load] : loads_) {
      std::cout << "#" << actor.id() << ": load: " << load << std::endl;
    }
    /* Implement a round robin with std::rotated, note that its complexity is
     * O(n). */
    std::vector<caf::actor> candidates(actors.size());
    rotated_ = (rotated_ + 1) % actors.size();
    auto pivot = actors.begin() + rotated_;
    std::rotate_copy(actors.begin(), pivot, actors.end(), candidates.begin());
    auto it = std::min_element(candidates.begin(), candidates.end(),
                               [this](auto &a, auto &b) {
                                 return GetWorkerLoad(a) < GetWorkerLoad(b);
                               });
    std::cout << " rotated: " << rotated_
              << " offset: " << it - candidates.begin() << std::endl;
    return *it;
  }

  bool Filter(caf::upgrade_lock<caf::detail::shared_spinlock> &guard,
              const caf::strong_actor_ptr &sender, caf::message_id mid,
              caf::message_view &mv, caf::execution_unit *eu);

  void Quit(caf::execution_unit *host) {
    // we can safely run our cleanup code here without holding
    // workers_mtx_ because abstract_actor has its own lock
    if (cleanup(planned_reason_, host)) unregister_from_system();
  }

  using Callers = std::unordered_map<caf::message_id, caf::actor>;
  Callers callers_;
  using Loads = std::unordered_map<caf::actor, size_t>;
  Loads loads_;

  size_t rotated_{std::numeric_limits<size_t>::max()};
  caf::detail::shared_spinlock workers_mtx_;
  std::vector<caf::actor> workers_;
  caf::exit_reason planned_reason_;
};

}  // namespace cdcf
#endif  // ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_LOAD_BALANCER_H_
