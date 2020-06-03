FROM gcc:latest AS builder

RUN sed -i "s@http://deb.debian.org@http://mirrors.aliyun.com@g" /etc/apt/sources.list
RUN apt-get clean \
    && apt-get update \
    && apt-get install python-pip cmake vim gdb -y
RUN pip install conan \
    && conan remote add inexorgame "https://api.bintray.com/conan/inexorgame/inexor-conan"

WORKDIR /cdcf

COPY conanfile.txt .
RUN conan install . -s compiler.libcxx=libstdc++11 --build missing

COPY CMakeLists.txt .
COPY config_manager config_manager
COPY node_keeper node_keeper
COPY actor_fault_tolerance actor_fault_tolerance
COPY actor_monitor actor_monitor
COPY actor_system actor_system
COPY logger logger
COPY demos demos
RUN cmake . -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake -DCMAKE_BUILD_TYPE=Release \
    && cmake --build . -j 8 \
    && ctest --output-on-failure

FROM debian
COPY --from=builder /cdcf/node_keeper/node_keeper /bin/node_keeper
COPY --from=builder /cdcf/demos/cluster/cluster /bin/cluster
COPY --from=builder /cdcf/demos/yanghui_cluster/yanghui_cluster_root_v2 /bin/yanghui_cluster_root_v2
COPY --from=builder /cdcf/demos/load_balancer/load_balancer /bin/load_balancer
COPY docker/script.sh /bin/script.sh
ENV APP=/bin/cluster
#ENTRYPOINT ["/bin/script.sh"]
CMD ["/bin/script.sh"]
