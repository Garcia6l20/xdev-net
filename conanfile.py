from conans import ConanFile, CMake, tools


class XDevNetConan(ConanFile):
    name = "xdev-net"
    version = "0.1.1"
    license = "MIT"
    author = "Garcia Sylvain <garcia dot 6l20 at gmail dot com>"
    url = "https://github.com/Garcia6l20/xdev-net"
    description = "Boost.Beast based http client/server"
    topics = ("networking", "http", "boost")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    generators = "cmake_paths"
    scm = {
     "type": "git",  # Use "type": "svn", if local repo is managed using SVN
     "url": "auto",
     "revision": "auto"
    }
    requires = ("boost/1.71.0@conan/stable", "openssl/1.0.2s", "zlib/1.2.11")
    default_options = {"shared": False, "boost:shared": False, "OpenSSL:shared": False}
    exports_sources = "include/*"
    # no_copy_source = True

    def build(self):
        cmake = CMake(self)

        run_tests = tools.get_env("CONAN_RUN_TESTS", False)

        cmake.definitions["XDEV_UNIT_TESTING"] = "ON" if run_tests else "OFF"

        cmake.configure()
        cmake.build()
        if run_tests:
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
