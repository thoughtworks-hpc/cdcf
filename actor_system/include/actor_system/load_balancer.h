/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_LOAD_BALANCER_H_
#define ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_LOAD_BALANCER_H_

#include <unordered_map>
#include <vector>

#include <caf/all.hpp>
#include <caf/default_attachable.hpp>

#include "actor_system/load_balancer/policy.h"

namespace cdcf {
namespace load_balancer {

struct Ticket {
  static Ticket ReplyTo(caf::mailbox_element_ptr &what) {
    bool required_reply = what->mid != what->mid.response_id();
    return required_reply ? Ticket{what->mid.response_id(),
                                   caf::actor_cast<caf::actor>(what->sender)}
                          : Ticket{};
  }

  caf::message_id response_id;
  caf::actor response_to;

  void Reply(caf::strong_actor_ptr sender, const caf::message &message,
             caf::execution_unit *host) const {
    if (response_to) {
      response_to->enqueue(sender, response_id, message, host);
    }
  }
};

class Router : public caf::monitorable_actor {
 public:
  static caf::actor make(caf::execution_unit *host,
                         load_balancer::Policy &&policy) {
    auto &sys = host->system();
    caf::actor_config config{host};
    auto res = caf::make_actor<Router, caf::actor>(sys.next_actor_id(),
                                                   sys.node(), &sys, config);
    auto abstract = caf::actor_cast<caf::abstract_actor *>(res);
    auto ptr = static_cast<Router *>(abstract);
    ptr->policy_ = std::move(policy);
    return res;
  }

 public:
  explicit Router(caf::actor_config &config)
      : caf::monitorable_actor(config),
        planned_reason_(caf::exit_reason::normal) {}

  void enqueue(caf::mailbox_element_ptr what,
               caf::execution_unit *host) override;

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
  using Lock = caf::upgrade_lock<caf::detail::shared_spinlock>;

  void Relay(caf::mailbox_element_ptr &what, caf::execution_unit *host,
             Lock &guard);

  void Reply(caf::mailbox_element_ptr &what, caf::execution_unit *host,
             Lock &guard);

  std::pair<caf::message_id, Ticket> PlaceTicket(
      caf::mailbox_element_ptr &what);

  Ticket ExtractTicket(caf::mailbox_element_ptr &what);

  caf::message_id AllocateRequestID(caf::mailbox_element_ptr &what);

  std::vector<Metrics> GetMetrics() const {
    std::vector<Metrics> result(workers_.size());
    std::transform(workers_.begin(), workers_.end(), result.begin(),
                   [&](auto &worker) {
                     auto it = metrics_.find(worker);
                     assert(it != metrics_.end());
                     return it->second;
                   });
    return result;
  }

  bool Filter(Lock &guard, caf::mailbox_element_ptr &what,
              caf::execution_unit *eu);

  void ClearWorker(Lock &guard);

  void DeleteWorker(Lock &guard, const caf::actor &worker);

  void AddWorker(Lock &guard, const caf::actor &worker);

  void Down(Lock &guard, caf::execution_unit *eu, const caf::down_msg &dm);

  void Exit(Lock &guard, caf::execution_unit *eu, const caf::exit_msg &message);

  caf::detail::shared_spinlock workers_mtx_;
  caf::exit_reason planned_reason_;
  caf::message_id last_request_id_ = {caf::make_message_id()};
  std::vector<caf::actor> workers_;
  std::unordered_map<caf::actor, Metrics> metrics_;
  std::unordered_map<caf::message_id, Ticket> tickets_;
  Policy policy_;
};

}  // namespace load_balancer
}  // namespace cdcf
#endif  // ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_LOAD_BALANCER_H_
