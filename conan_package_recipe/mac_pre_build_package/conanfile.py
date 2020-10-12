from conans import ConanFile, CMake, tools

class CdcfConan(ConanFile):
    name = "cdcf"
    version = "1.0"
    license = "MIT"
    author = "thoughtworks HPC"
    url = "https://github.com/thoughtworks-hpc/cdcf"
    description = "HPC Internal Project"
    topics = ("actor model", "distributed computing framework", "caf")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    generators = "cmake"

    def source(self):
        self.run("git clone -b develop https://github.com/thoughtworks-hpc/cdcf")
        tools.replace_in_file("cdcf/CMakeLists.txt", "project(cdcf)",
                              '''project(cdcf)
        include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
        conan_basic_setup()''')

    def requirements(self):
        self.requires("caf/0.17.3@bincrafters/stable")
        self.requires("gtest/1.10.0")
        self.requires("asio/1.13.0")
        self.requires("protobuf/3.9.1@bincrafters/stable")
        self.requires("grpc/1.25.0@inexorgame/stable")
        self.requires("spdlog/1.4.2")

    def build(self):
        cmake = CMake(self)
        self.run('cd cdcf && conan install . -s compiler.libcxx=libc++ --build missing')
        self.run('cmake %s/cdcf %s'
                 % (self.source_folder, cmake.command_line + '  -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake '))
        self.run("cmake --build . %s -- -j 4" % cmake.build_config)

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["actor_system", "common"]
