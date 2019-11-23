# xdev-http-server

> HTTP server based on Boost.Beast library

## Build from source

```bash
mkdir build && cd build
conan install ..
cmake -DCMAKE_TOOLCHAIN_FILE=./conan_paths.cmake ..
make
```
