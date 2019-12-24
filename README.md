# CDCF: C++ Distributed Computing Framework

## Project Hierarchy

```text
root
.
├── CMakeLists.txt
├── actor_system            ....... Library to build actor system
│   ├── CMakeLists.txt
│   ├── include             .......   Exported headers
│   └── src                 .......   Implementation and testing for actor system
├── demos                   ....... Folders holding demos
│   ├── CMakeLists.txt
│   └── hello_world         .......   Folders holding per demo
│       └── CMakeLists.txt
└── node_keeper             ....... Executable to manage nodes in cluster
    ├── CMakeLists.txt
    └── src                 .......   Implementation and testing for node keeper
```
