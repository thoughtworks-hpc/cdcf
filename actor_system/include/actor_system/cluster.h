/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_CLUSTER_H_
#define ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_CLUSTER_H_
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace actor_system {
namespace cluster {
struct Member {
  std::string host;
  uint16_t port;
  enum Status { Up, Down };
  Status status{Status::Up};
  bool operator==(const struct Member& other) const {
    return host == other.host && port == other.port;
  }
};

struct Event {
  Member member;
};

class Observer {
 public:
  virtual void Update(const Event& event) = 0;
};

class Subject {
 public:
  using ObserverPtr = Observer*;
  void AddObserver(ObserverPtr observer) {
    std::lock_guard lock(observers_mutex_);
    observers_.push_back(observer);
  }

  void RemoveObserver(ObserverPtr observer) {
    std::lock_guard lock(observers_mutex_);
    observers_.remove(observer);
  }

  void Notify(const Event& event) {
    std::lock_guard lock(observers_mutex_);
    for (auto observer : observers_) {
      observer->Update(event);
    }
  }

 private:
  std::mutex observers_mutex_;
  std::list<ObserverPtr> observers_;
};

class ClusterImpl;

class Cluster : public Subject {
  static std::mutex instance_mutex_;
  static std::unique_ptr<Cluster> instance_;

  Cluster(const Cluster&) = delete;
  Cluster(const Cluster&&) = delete;

 public:
  static Cluster* GetInstance();

  ~Cluster();

  std::vector<Member> GetMembers();

 private:
  Cluster();

  std::unique_ptr<ClusterImpl> impl_;
};
};  // namespace cluster
}  // namespace actor_system
#endif  // ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_CLUSTER_H_
