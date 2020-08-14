#CDCF Develop Document

##Introduction
CDCF full name is C++ Distributed Computing Framework. It was born for high performance. 
It is to fill in the gap that there was no C++ distributed system development framework before.

CDCF consists of three parts：node keeper，monitor tool， actor system

Node keeper is an executable program that needs to be deployed on each node of a distributed cluster. It is responsible for managing all node members of the cluster.At the same time, it is also responsible for restarting the actor system when the actor system is hung.

Monitor tool is an executable program responsible for monitoring the running state of the cluster. It only needs to be deployed on one node. Users can know the CPU / memory usage and the number of actors of each node in the cluster through the monitor tool.

Actor system is composed of a series of C++ APIs. Users can use these APIs to develop distributed business based on actor model distributed system.

###CDCF dependence 
CAF(C++ actor framework): CDCF Actor system API use the CAF libs.

ASIO: Node keeper connect use ASIO

grpc/Protobuff: Node keeper communicate with actor system.

spdlog: CDCF logger use spdlog

gTest: CDCF 

###CDCF run environment

Linux OS, docker 19.03(optional)

###CDCF develop environment

Linux OS: gcc 8.4, cmake 3.10, conan 1.24, git

Windows OS, VS2019, cmake 3.10, conan 1.24, git

Mac OS, clang 11.0.0, cmake 3.10, conan 1.24, git

###Architecture diagram
![CDCF_architecture](./image/CDCF_architecture.jpg)

##Getting Started

Down load CDCF code

```shell script
git clone https://github.com/thoughtworks-hpc/cdcf.git
cd cdcf
```

Config Conan
```
conan remote add inexorgame "https://api.bintray.com/conan/inexorgame/inexor-conan"
mkdir build && cd build
conan install .. --build missing
```

Build Project
```shell script
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake
cmake --build . -j
```

Test
```shell script
ctest  --output-on-failure
```

##Tour of CDCF

This tour will show how to develop a simple ping pong cluster via CDCF.
You can get this tour resource code in "cdcf/demo/simple_ping_pong"

Define the cluster run config

```c++
struct PingPongConfig : public actor_system::Config {
  std::string pong_host = "127.0.0.1";
  uint16_t pong_port = 58888;

  PingPongConfig() {
    opt_group{custom_options_, "global"}
        .add(pong_host, "pong_host", "set pong host")
        .add(pong_port, "pong_port", "set pong port");
  }
};
```

Define the pong actor, receive a value print it and return value + 1
```c++
caf::behavior pong_fun(caf::event_based_actor* self) {
  return {
      [=](int ping_value) -> int{
        CDCF_LOGGER_INFO("Get ping_value:", ping_value);
        return ++ping_value;
      },
  };
}
```

Define the ping actor, send 0 value to pong actor when start, receive a value print it and return value + 1
```c++
caf::behavior ping_fun(caf::event_based_actor* self, const caf::actor& pong_actor) {
  self->send(pong_actor,  0);
  return {
      [=](int pong_value) -> int {
        CDCF_LOGGER_INFO("Get pong value:{}", pong_value);
        return ++pong_value;
      },
  };
}
```

start pong actor and publish it, so anther node can send message to it, and add the node to cluster, notify cluster start ready
```c++

//this struct will use to add cluster
class Pong : public actor_system::cluster::Observer  {
 public:
  void Update(const actor_system::cluster::Event& event) override {
  }

  Pong() {
  }
};

//the main function of application
void caf_main(caf::actor_system& system, const PingPongConfig& cfg) {
  Pong* pong = new Pong();
  system.spawn(pong_fun);

  //add node to cluster
  actor_system::cluster::Cluster::GetInstance()->AddObserver(pong);

  //start pong actor
  auto pong_actor = system.spawn(pong_fun);

  //publish the pong actor, so remote node can send message to it
  system.middleman().publish(pong_actor, cfg.pong_port);
  CDCF_LOGGER_INFO("pong_actor start at port:{}", cfg.pong_port);

  //notify cluster the application is start read
  actor_system::cluster::Cluster::GetInstance()->NotifyReady();
}

//this code is necessary
CAF_MAIN(caf::io::middleman)
```

Start ping node, when pong node up, connect pong node
```c++
class Ping : public actor_system::cluster::Observer {
 public:
  void Update(const actor_system::cluster::Event& event) override {
      if (event.member.name == "Pong"){
        auto remote_pong = system_.middleman().remote_actor(config_.pong_host, config_.pong_port);
        if (nullptr == remote_pong){
          CDCF_LOGGER_ERROR("get remote pong error");
        }

        system_.spawn(ping_fun, *remote_pong);
        CDCF_LOGGER_INFO("Start ping actor");
      }
  }
  Ping(caf::actor_system& system, const PingPongConfig& config):system_(system),config_(config) {  }

 private:
  caf::actor_system& system_;
  const PingPongConfig& config_;
};



void caf_main(caf::actor_system& system, const PingPongConfig& cfg) {
  Ping* ping = new Ping(system, cfg);
  actor_system::cluster::Cluster::GetInstance()->AddObserver(ping);
  actor_system::cluster::Cluster::GetInstance()->NotifyReady();
}

CAF_MAIN(caf::io::middleman)
```



##How to Use Yanghui demo
The Yanghui demo use all CDCF API to develop, So analysing Yanghui demo Code can help you know how to use 
CDCF to develop your business. You can read Chapter 7 in combination with this chapter.

###run Yanghui demo
Make sure in CDCF folder
```shell script
cd cdcf
```

Build docker image 
```shell script
docker build -t cdcf .
```

Deploy Yanghui cluster with docker
```shell script
cd docker
docker-compose -f yanghui_app.yml up 
```

Run Yanghui demo application on Cluster
```shell script
docker exec -it docker_yanghui_root_v2_1 /bin/script.sh
```

After that, you can input 'n' to run Yanghui triangle count and get result.

###Yanghui demo code
Yanghui demo code is in folder "cdcf/demos/yanghui_cluster". The demo use all CDCF feature to implement Yanghui triangle 
 min path in several way.
 
The main function is in file "yanghui_example_v2.cc".

The Yanghui cluster has two role: root and worker. 
root response for record processing result send compute command to worker and deal to result returned from worker.

The worker is responsible for calculating the addition and the minimum number.

The root enter function is "SmartRootStart" and the work enter function is "SmartWorkerStart"

All cluster config implement code is in file "yanghui_config.h, you can understand how to config Yanghui cluster via
 this file. You can understand how to deploy the Yanghui cluster via file "cdcf/docker/yanghui_app.yml".

##Node Keeper Deploy
Node keeper is an executable program that needs to be deployed on each node of a distributed cluster. It is responsible 
for managing all node members of the cluster. At the same time, it is also responsible for restarting the actor system
when the actor system is hung. 

Run Node keeper like command

```shell script
node_keeper --name=${NAME} --seeds=${SEEDS} --host=${HOST} --port=${PORT} --role=${ROLE} --app=${APP} --app-args="${APP_ARGS}"
```

${NAME}: node name, you can get the name via CDCF node member management API, so you can identify the node in you business 
code.

${SEEDS}: seed nodes of cluster, cluster use the seed nodes to manage cluster member， it's a list with format "host1:port1,host2:port2"

${HOST}: current host ip or domain name

${PORT}：current host port

${APP}: actor system executable 

${APP_ARGS}： actor system executable run parameters

More advance Node Keeper parameters can get by --help

```
node_keeper --help
```


##Monitor Tool
###Get All Cluster Node Run Status
CDCF monitor tool can get all nodes of cluster status. 
 
```shell script
cluster_monitor_client -S ${HOST}:${PORT}
```

The ${HOST} and ${PORT} is the same parameter of node_keeper. Input any node host and port can get all cluster node status
 info.
 
For example in Yanghui cluster demo input 

```shell script
cluster_monitor_client -S yanghui_root_v2:50051
```

And will get all cluster nodes information as below 

```shell 
Get cluster node status from:yanghui_root_v2:50051
Total node:3
Cluster node ip: 172.21.0.2
  Node name: yanghui_worker2_v2
  Node role: worker
  Cpu use rate: 0%
  Memory use rate: 25.1372%
  Max memory: 2038544kB
  Use memory: 512432kB

Cluster node ip: 172.21.0.4
  Node name: yanghui_root_v2
  Node role: root
  Cpu use rate: 0%
  Memory use rate: 25.1625%
  Max memory: 2038544kB
  Use memory: 512948kB

Cluster node ip: 172.21.0.5
  Node name: yanghui_worker1_v2
  Node role: worker
  Cpu use rate: 0%
  Memory use rate: 25.1625%
  Max memory: 2038544kB
  Use memory: 512948kB
```

###Get One Node Actor Information

CDCF monitor tool can get one node actor information by command
```shell script
cluster_monitor_client -N ${HOST}
```


For example in Yanghui cluster demo input 
```shell script
cluster_monitor_client -N yanghui_worker2_v2
```

Get the one yanghui_worker2 node information 
```
Get actor system status from: yanghui_worker2_v2:50052

Actor executor: 6
Total actor: 7

 actor: id=(21)
  name: pool name
  description: pool description

 actor: id=(9)
  name: calculator2
  description: a actor can calculate for yanghui.

 actor: id=(7)
  name: calculator1
  description: a actor can calculate for yanghui.

 actor: id=(11)
  name: calculator3
  description: a actor can calculate for yanghui.

 actor: id=(13)
  name: calculator for load balance
  description: a actor can calculate for load balance yanghui.

 actor: id=(20)
  name: pool name
  description: pool description

 actor: id=(19)
  name: pool name
  description: pool description
``` 











