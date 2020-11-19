from conans import ConanFile, CMake, tools

class CdcfConan(ConanFile):
    name = "cdcf"
    version = "1.2.2"
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
        self.requires("grpc/1.27.3@inexorgame/stable")
        self.requires("spdlog/1.4.2")
        self.requires("openssl/1.0.2u")

    def build(self):
        cmake = CMake(self)
        if self.settings.os == "Linux":
            # self.run('mkdir unzipFolder')
            # tools.unzip('%s/cdcf/mac_cdcf_1_2_1_hpc_stable_package.zip' % (self.source_folder), 'unzipFolder')
            self.run('cd cdcf && conan install . -s compiler.libcxx=libstdc++11 --build missing')
        else:
            self.run('cd cdcf && conan install . -s compiler.libcxx=libc++ --build missing')

        self.run('cmake %s/cdcf %s'
            % (self.source_folder, cmake.command_line + '  -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake '))
        self.run("cmake --build . %s -- -j 4" % cmake.build_config)

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["actor_system", "common"]

