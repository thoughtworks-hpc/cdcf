cmake_minimum_required(VERSION 3.10)
project(yanghui_cluster)


add_executable(${PROJECT_NAME}_root_v2 yanghui_example_v2.cc count_cluster.cc actor_union_count_cluster.cc yanghui_actor.cc yanghui_demo_calculator.cc
        cdcf_spawn.cc
        router_pool_count_cluster.cc yanghui_sample_actor.cc simple_counter.cc
        yanghui_with_priority.cc
        yanghui_server.cc)

target_link_libraries(${PROJECT_NAME}_root_v2 common actor_system)

add_executable(yanghui_actor_test yanghui_actor_test.cc count_cluster.cc actor_union_count_cluster.cc yanghui_actor.cc fake_counter.cc
        yanghui_demo_calculator.cc yanghui_with_priority.cc yanghui_server.cc)

target_link_libraries(yanghui_actor_test common actor_system)

add_executable(worker_test worker_test.cc yanghui_demo_calculator.cc yanghui_with_priority.cc yanghui_server.cc)
target_link_libraries(worker_test common actor_system)

add_executable(multiple_actor_test simple_counter.cc yanghui_multiple_actor_test.cc yanghui_sample_actor.cc yanghui_demo_calculator.cc yanghui_with_priority.cc yanghui_server.cc)
target_link_libraries(multiple_actor_test common actor_system)


