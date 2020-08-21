# CDCF: C++ Distributed Computing Framework

![](https://github.com/thoughtworks-hpc/cdcf/workflows/CI/badge.svg)
![](https://github.com/thoughtworks-hpc/cdcf/workflows/Coding%20Style/badge.svg)

## Build from source

Requires:

- CMake 3.10 above
- conan 1.21 above

Checkout to project root and run following commands:

```shell
$ conan remote add inexorgame "https://api.bintray.com/conan/inexorgame/inexor-conan"
$ mkdir build && cd build
$ conan install .. --build missing
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake
$ cmake --build . -j
$ ctest .
$ cpack
```

cpack will generate installation package in dir `pack`.
