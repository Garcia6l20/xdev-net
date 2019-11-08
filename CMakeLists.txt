cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS -fconcepts)

project(net)
add_library(${PROJECT_NAME}
    include/net/net.hpp
    include/net/unique_function.hpp
    include/net/error.hpp
    include/net/address.hpp
    include/net/socket.hpp
    include/net/udp.hpp
    include/net/tcp.hpp

    src/net.cpp
    src/error.cpp
    src/address.cpp
    src/socket.cpp
    src/udp.cpp
    src/tcp.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC include)

add_subdirectory(tests)
