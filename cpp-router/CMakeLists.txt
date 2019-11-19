cmake_minimum_required(VERSION 3.7)

project(xdev-router)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(Boost COMPONENTS system REQUIRED)

add_library(${PROJECT_NAME}
    include/router.hpp
    include/router_details.hpp
    include/route_parameter.hpp
    src/router.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE include inline)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::boost Boost::system)
add_library(XDev::Router ALIAS ${PROJECT_NAME})

option(XDEV_ROUTER_BUILD_TESTS "Build tests for xdev-router" ON)
if (XDEV_ROUTER_BUILD_TESTS)
    find_package(GTest REQUIRED)
    include(CTest)
    add_subdirectory(tests)
endif()

