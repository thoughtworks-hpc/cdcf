/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include <daemon.h>

#include <vector>

#include "src/node_keeper.h"

std::vector<std::string> ConstructAppArgs(const node_keeper::Config& config);

int main(int argc, char* argv[]) {
  node_keeper::Config config;
  auto ret = config.parse_config(argc, argv, "cdcf-default.ini");
  if (ret != CDCFConfig::RetValue::kSuccess) {
    return 1;
  }

  cdcf::Logger::Init(config);

  auto seeds = config.GetSeeds();
  if (seeds.size()) {
    std::cout << "seeding with: " << std::endl;
    for (size_t i = 0; i < seeds.size(); ++i) {
      std::cout << "\t" << seeds[i].host << ":" << seeds[i].port << std::endl;
    }
  }
  node_keeper::NodeKeeper keeper(config);

  if (!config.single_run_) {
    PosixProcessManager process_manager;
    auto args = ConstructAppArgs(config);
    Daemon daemon(
        process_manager, config.app_, args,
        [&keeper]() { keeper.NotifyActorSystemDown(); },
        [&keeper]() { keeper.NotifyLeave(); });
    daemon.Start();
  }

  keeper.Run();
  return 0;
}

std::vector<std::string> ConstructAppArgs(const node_keeper::Config& config) {
  std::vector<std::string> args{"--host=" + config.host_,
                                "--name=" + config.name_};

  if (!config.role_.empty()) {
    args.push_back("--role=" + config.role_);
  }

  auto parsed = node_keeper::split(config.app_args_, ' ');
  args.insert(args.end(), parsed.begin(), parsed.end());

  std::cout << "app-args:" << std::endl;
  std::cout << "app-arpg len: " << args.size() << std::endl;
  std::for_each(args.begin(), args.end(),
                [](auto& arg) { std::cout << arg << std::endl; });

  return args;
}
