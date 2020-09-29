from conans import ConanFile, CMake, tools

class CdcfConan(ConanFile):
    name = "cdcf"
    version = "1.1"
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
        if self.settings.os == "Linux":
            self.run("git clone -b develop https://github.com/thoughtworks-hpc/cdcf")
            tools.replace_in_file("cdcf/CMakeLists.txt", "project(cdcf)",
                                  '''project(cdcf)
        include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
        conan_basic_setup()''')

        if self.settings.os == "Macos":
            self.run("git clone -b conan_package https://github.com/thoughtworks-hpc/cdcf")

    def requirements(self):
        self.requires("caf/0.17.3@bincrafters/stable")
        self.requires("gtest/1.10.0")
        self.requires("asio/1.13.0")
        self.requires("protobuf/3.9.1@bincrafters/stable")
        self.requires("grpc/1.25.0@inexorgame/stable")
        self.requires("spdlog/1.4.2")

    def build(self):
        if self.settings.os == "Macos":
            self.run('mkdir unzipFolder')
            tools.unzip('%s/cdcf/mac_cdcf_stable_package.zip' % (self.source_folder), 'unzipFolder')
        else:
            cmake = CMake(self)
            if self.settings.os == "Linux":
                self.run('cd cdcf && conan install . -s compiler.libcxx=libstdc++11 --build missing')

            self.run('cmake %s/cdcf %s'
                % (self.source_folder, cmake.command_line + '  -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake '))
            self.run("cmake --build . %s -- -j 4" % cmake.build_config)

    def package(self):
        if self.settings.os == "Linux":
            cmake = CMake(self)
            cmake.install()
        if self.settings.os == "Macos":
            self.copy("*", dst="include", src="unzipFolder/include")
            self.copy("*", dst="bin", src="unzipFolder/bin")
            self.copy("*", dst="lib", src="unzipFolder/lib")

    def package_info(self):
        self.cpp_info.libs = ["actor_system", "common"]

