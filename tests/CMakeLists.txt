include(FetchContent)

FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.10.0
)
FetchContent_MakeAvailable(googletest)

link_libraries(${PROJECT_NAME} gtest gtest_main gmock)
include_directories(${CMAKE_CURRENT_LIST_DIR})

project(${PROJECT_NAME}-test)

find_program(VALGRIND_EXE valgrind)
if (VALGRIND_EXE)
    set(CTEST_MEMORYCHECK_COMMAND "${VALGRIND_EXE}")
endif()

macro(add_simple_test _name)
    add_executable(${PROJECT_NAME}-${_name} ${_name}.cpp)
    add_test(NAME ${PROJECT_NAME}-${_name} COMMAND ${PROJECT_NAME}-${_name})
endmacro()

macro(auto_add_simple_tests)
    file(GLOB _files *.cpp)
    foreach(_file ${_files})
        get_filename_component(_name ${_file} NAME_WE)
        add_simple_test(${_name})
    endforeach()
endmacro()

add_subdirectory(traits)
add_subdirectory(url)
# add_subdirectory(router)
add_subdirectory(http)
