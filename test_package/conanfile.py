import os

from conans import ConanFile, CMake, tools


class ComputeTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    build_requires = "gtest/1.10.0"

    def build(self):
        cmake = CMake(self)
        # Current dir is "test_package/build/<build_id>" and CMakeLists.txt is
        # in "test_package"
        cmake.configure()
        cmake.build()

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")
        self.copy('*.so*', dst='bin', src='lib')

    def test(self):
        if not tools.cross_building(self):
            os.chdir("bin")
            print("Running test binary...")
            self.run(".%scompute_test" % os.sep)
            print("Running demo with 3 threads...")
            self.run(".%sthread_demo --threads 3 < ../../../program.thd" % os.sep)
