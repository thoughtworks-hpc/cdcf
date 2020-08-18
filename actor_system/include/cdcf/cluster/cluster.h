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

#include <caf/all.hpp>

namespace cdcf::cluster {
struct Member {
  std::string name;

  std::string hostname;
  std::string role;
  std::string host;
  uint16_t port;
  enum Status { Up, Down, ActorSystemDown, ActorSystemUp };
  Status status{Status::Up};

  Member(const std::string& name, const std::string& hostname,
         const std::string& host, const std::string& role, uint16_t port,
         Status status = Up)
      : name(name),
        hostname(hostname),
        host(host),
        role(role),
        port(port),
        status(status) {}

  bool operator==(const struct Member& other) const {
    return host == other.host && port == other.port;
  }

  bool operator<(const Member& rhs) const {
    if (host < rhs.host) return true;
    if (rhs.host < host) return false;
    return port < rhs.port;
  }
};

struct Actor {
  std::string address;

  bool operator<(const Actor& rhs) const { return address < rhs.address; }
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
  static Cluster* GetInstance(const std::string& host_ip, uint16_t port);

  ~Cluster();

  std::vector<Member> GetMembers();

  void NotifyReady();

 private:
  Cluster();
  Cluster(const std::string& host_ip, uint16_t port);

  std::unique_ptr<ClusterImpl> impl_;
};
};      // namespace cdcf::cluster
#endif  // ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_CLUSTER_H_
