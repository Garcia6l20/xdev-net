cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /std:c++latest")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts")
endif()

project(net)

include(FetchContent)
include(CheckIncludeFile)

function(make_patch_command _out_command _dir)
    set(${_out_command} "")
    file(GLOB _patches ${_dir}/*.patch)
    foreach (_patch ${_patches})
        list(APPEND ${_out_command} "git apply ${_patch}")
    endforeach()
    list(JOIN ${_out_command} " && " ${_out_command})
endfunction()

make_patch_command(nodejs-http-parser-patch_cmd ${PROJECT_SOURCE_DIR}/patches/jthread)
FetchContent_Declare(nodejs-http-parser
    GIT_REPOSITORY https://github.com/nodejs/http-parser.git
    GIT_TAG v2.9.2
    PATCH_COMMAND ${nodejs-http-parser-patch_cmd}
)
FetchContent_MakeAvailable(nodejs-http-parser)

check_include_file(jthread _HAS_JTREAD)
if(NOT _HAS_JTREAD)
    make_patch_command(jthread_patch_cmd ${PROJECT_SOURCE_DIR}/patches/jthread)
    FetchContent_Declare(jthread
            GIT_REPOSITORY https://github.com/josuttis/jthread.git
            GIT_TAG 8478824deefdb2268be9b9a873827ba396587f7f
            PATCH_COMMAND ${jthread_patch_cmd}
    )
    FetchContent_MakeAvailable(jthread)
    list(APPEND ${PROJECT_NAME}_EXTRA_INCLUDE_DIR ${jthread_SOURCE_DIR}/source)
endif()

configure_file(include/net/net-config.hpp.in include/net/net-config.hpp @ONLY)
list(APPEND ${PROJECT_NAME}_EXTRA_INCLUDE_DIR ${PROJECT_BINARY_DIR}/include)

add_library(${PROJECT_NAME} STATIC
    include/net/net.hpp
    include/net/net-config.hpp.in
    ${PROJECT_BINARY_DIR}/include/net/net-config.hpp
    include/net/unique_function.hpp
    include/net/error.hpp
    include/net/address.hpp
    include/net/socket.hpp
    include/net/udp.hpp
    include/net/tcp.hpp
    include/net/http_parser.hpp
    include/net/http_server.hpp
    include/net/http_request.hpp

    src/net.cpp
    src/error.cpp
    src/address.cpp
    src/socket.cpp
    src/udp.cpp
    src/tcp.cpp
    src/http_server.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC include ${${PROJECT_NAME}_EXTRA_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC nodejs-http-parser)

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC wsock32 ws2_32 debug msvcrtd)
endif()

enable_testing()
add_subdirectory(tests)
include(CTest)
