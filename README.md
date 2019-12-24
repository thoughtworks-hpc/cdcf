# CDCF: C++ Distributed Computing Framework

![](https://github.com/thoughtworks-hpc/cdcf/workflows/CI/badge.svg)

## Build from source

Requires:

- CMake 3.10 above
- conan 1.21 above

Checkout to project root and run following commands:

```shell
$ conan install .
$ cmake . -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake
$ cmake --build .
$ ctest .
```
