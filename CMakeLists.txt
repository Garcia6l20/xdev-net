cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts")
endif()

project(net)
add_library(${PROJECT_NAME} STATIC
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

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC wsock32 ws2_32)
endif()

add_subdirectory(tests)
