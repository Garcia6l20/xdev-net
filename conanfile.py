from conans import ConanFile, CMake, tools


class XDevNetConan(ConanFile):
    name = "xdev-net"
    version = "0.1"
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Xdevnet here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = "cmake_paths"
    scm = {
     "type": "git",  # Use "type": "svn", if local repo is managed using SVN
     "url": "auto",
     "revision": "auto"
    }
    requires = ("boost/1.71.0@conan/stable")
    build_requires = ("gtest/1.8.1@bincrafters/stable")
    exports_sources = "include/*"
    # no_copy_source = True

    def config(self):
        self.settings.compiler.cppstd = '20'

    def build(self):
        cmake = CMake(self)

        cmake.definitions["XDEV_UNINT_TESTING"] = "On" if tools.get_env("CONAN_RUN_TESTS", False) else "Off"

        cmake.configure()
        cmake.build()
        if tools.get_env("CONAN_RUN_TESTS", False):
            cmake.test()

    def package(self):
        self.copy("*.hpp", dst="include", src="include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["xdev-net"]
