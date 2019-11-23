cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /std:c++latest")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts")
endif()

project(xdev-net)

find_package(Boost 1.71.0 COMPONENTS system coroutine thread REQUIRED)
find_package(Threads REQUIRED)

configure_file(include/xdev/net/config.hpp.in ${PROJECT_BINARY_DIR}/include/xdev/net/config.hpp)

set(${PROJECT_NAME}-headers
    include/xdev/net.hpp
    ${PROJECT_BINARY_DIR}/include/xdev/net/config.hpp
    include/xdev/net/function_traits.hpp
    include/xdev/net/http_server.hpp
    include/xdev/net/http_client.hpp
    include/xdev/net/router.hpp
    include/xdev/net/router_details.hpp
    include/xdev/net/route_parameter.hpp
)

add_library(${PROJECT_NAME} STATIC
    ${${PROJECT_NAME}-headers}

    include/xdev/net/config.hpp.in
    src/xdev-net.cpp
)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    message(STATUS "Using precompiled headers")
    target_precompile_headers(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}-headers})
endif()

target_include_directories(${PROJECT_NAME} PUBLIC include ${${PROJECT_NAME}_EXTRA_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads Boost::headers Boost::system Boost::coroutine Boost::thread)

target_link_libraries(${PROJECT_NAME} PUBLIC ssl crypto)

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC wsock32 ws2_32 debug msvcrtd)
endif()

option(XDEV_UNINT_TESTING "Enable unit testing" ON)
if(XDEV_UNINT_TESTING)
  enable_testing()
  add_subdirectory(tests)
  include(CTest)
endif()
