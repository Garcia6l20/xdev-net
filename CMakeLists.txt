cmake_minimum_required(VERSION 3.14)

# available options
option(XDEV_UNIT_TESTING "Enable unit testing" ON)
option(XDEV_USE_MBEDTLS "Enable mbedtls ssl backend" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /std:c++latest /bigobj")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts")
endif()

project(xdev-net)

option(BOOST_MBEDED "Embed boost with ExternalProject" ON)
if (BOOST_MBEDED)
    message("Using embeded boost")
    include(./CMake/GetBoost.cmake)
else()
    set(Boost_USE_STATIC_LIBS ON)
    find_package(Boost 1.71.0 COMPONENTS system coroutine thread date_time REQUIRED)
endif()

find_package(Threads REQUIRED)

if(NOT XDEV_USE_MBEDTLS)
    find_package(OpenSSL)
endif()

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

if (FALSE) # ${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    message(STATUS "Using precompiled headers")
    target_precompile_headers(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}-headers})
endif()

if(XDEV_USE_MBEDTLS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DBOOST_ASIO_USE_MBEDTLS=1)
    set(MBEDTLS_LIBRARIES mbedtls mbedcrypto mbedx509)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC include ${${PROJECT_NAME}_EXTRA_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PUBLIC ${Boost_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${MBEDTLS_LIBRARIES})

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC wsock32 ws2_32 debug msvcrtd)
endif()

if(XDEV_UNIT_TESTING)
  enable_testing()
  add_subdirectory(tests)
  include(CTest)
endif()
