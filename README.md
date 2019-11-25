# xdev-http-server

> HTTP server based on Boost.Beast library

## Development

## Building

```bash
mkdir build && cd build
conan install .. -s compiler.cppstd=20
cmake -DCMAKE_TOOLCHAIN_FILE=./conan_paths.cmake ..
make
```

## For usage

> Please note that c++20 is required
This implies using -std=c++20

```bash
conan create . -s compiler.cppstd=20
```
