from conans import ConanFile, CMake, tools
import os


class ComputeConan(ConanFile):
    name = "compute"
    version = "1.0.0"
    license = "BSD-2-Clause"
    author = "Cornel Izbasa"
    url = "https://github.com/Mr-Slippery/threads-demo"
    description = "C++ thread demo with controllers and configurable number of workers and stub computatinal payload"
    topics = ("cpp", "threads", "compute")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"
    exports_sources = "compute/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="compute")
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src="compute/include")
        self.copy("*compute.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["compute"]
