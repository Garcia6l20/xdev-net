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

macro(make_patch_command _out_command _dir)
    set(${_out_command} "")
    file(GLOB _patches ${_dir}/*.patch)
    foreach (_patch ${_patches})
        list(APPEND ${_out_command} "git apply ${_patch}")
    endforeach()
    list(JOIN ${_out_command} " && " ${_out_command})
	if (UNIX)
		file(WRITE ${PROJECT_BINARY_DIR}/${_out_command}.sh ${${_out_command}})
		set(${_out_command} bash ${PROJECT_BINARY_DIR}/${_out_command}.sh)
	endif()
endmacro()

include(./CMake/GetBoost.cmake)

configure_file(include/net/net-config.hpp.in include/net/net-config.hpp @ONLY)
list(APPEND ${PROJECT_NAME}_EXTRA_INCLUDE_DIR ${PROJECT_BINARY_DIR}/include)

add_library(${PROJECT_NAME} STATIC
    include/net/net.hpp
    include/net/net-config.hpp.in
    ${PROJECT_BINARY_DIR}/include/net/net-config.hpp
    include/net/function_traits.hpp
    include/net/http_server.hpp
    include/net/http_client.hpp
    include/net/router.hpp
    include/net/router_details.hpp
    include/net/route_parameter.hpp

    src/net.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC include ${${PROJECT_NAME}_EXTRA_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PUBLIC ${Boost_LIBRARY_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})
add_dependencies(${PROJECT_NAME} boost)

target_link_libraries(${PROJECT_NAME} PUBLIC ssl crypto)

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC wsock32 ws2_32 debug msvcrtd)
endif()

enable_testing()
add_subdirectory(tests)
include(CTest)
