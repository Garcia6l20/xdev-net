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

add_subdirectory(traits)
add_subdirectory(socket)
add_subdirectory(http)
