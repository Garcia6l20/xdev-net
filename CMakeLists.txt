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

include(FetchContent)

FetchContent_Declare(nodejs-http-parser
    GIT_REPOSITORY https://github.com/nodejs/http-parser.git
    GIT_TAG v2.9.2
	PATCH_COMMAND git apply ${PROJECT_SOURCE_DIR}/patches/nodejs-http-parser/0001-add-cmakelists.patch
)
FetchContent_MakeAvailable(nodejs-http-parser)

add_library(${PROJECT_NAME} STATIC
    include/net/net.hpp
    include/net/unique_function.hpp
    include/net/thread_guard.hpp
    include/net/error.hpp
    include/net/address.hpp
    include/net/socket.hpp
    include/net/udp.hpp
    include/net/tcp.hpp
    include/net/http_server.hpp

    src/net.cpp
    src/error.cpp
    src/address.cpp
    src/socket.cpp
    src/udp.cpp
    src/tcp.cpp
    src/http_server.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PUBLIC nodejs-http-parser)

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC wsock32 ws2_32)
endif()

enable_testing()
add_subdirectory(tests)
include(CTest)
