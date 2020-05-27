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

COPY . .
RUN cmake . -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake -DCMAKE_BUILD_TYPE=Release \
    && cmake --build . \
    && ctest --output-on-failure

FROM debian
COPY --from=builder /cdcf/node_keeper/node_keeper /bin/node_keeper
COPY --from=builder /cdcf/demos/cluster/cluster /bin/cluster
COPY docker/script.sh /bin/script.sh
ENV APP=/bin/cluster
ENTRYPOINT ["/bin/script.sh"]
