cmake_minimum_required(VERSION 3.10)
project(yanghui_cluster)

find_package(caf REQUIRED)
message("==========================================================")
message("================= ${caf_INCLUDE_DIRS} ====================")
message("==========================================================")
include_directories(${caf_INCLUDE_DIRS})
include_directories(../../config_manager/include)


add_executable(${PROJECT_NAME}_root_v2 yanghui_example_v2.cc count_cluster.cc balance_count_cluster.cc actor_union_count_cluster.cc yanghui_actor.cc yanghui_demo_calculator.cc
        cdcf_spawn.cc
        router_pool_count_cluster.cc yanghui_sample_actor.cc simple_counter.cc)

target_link_libraries(${PROJECT_NAME}_root_v2 caf::caf)
target_link_libraries(${PROJECT_NAME}_root_v2 config_manager actor_monitor actor_fault_tolerance actor_system logger router_pool)

add_executable(yanghui_actor_test yanghui_actor_test.cc count_cluster.cc balance_count_cluster.cc actor_union_count_cluster.cc yanghui_actor.cc fake_counter.cc
        yanghui_demo_calculator.cc)

target_link_libraries(yanghui_actor_test caf::caf config_manager actor_monitor actor_fault_tolerance actor_system logger)

add_executable(worker_test worker_test.cc yanghui_demo_calculator.cc)
target_link_libraries(worker_test caf::caf config_manager actor_monitor actor_fault_tolerance actor_system logger)

add_executable(multiple_actor_test simple_counter.cc yanghui_multiple_actor_test.cc yanghui_sample_actor.cc yanghui_demo_calculator.cc)
target_link_libraries(multiple_actor_test caf::caf config_manager actor_monitor actor_fault_tolerance actor_system logger)


