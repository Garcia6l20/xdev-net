find_package(GTest REQUIRED)

link_libraries(${PROJECT_NAME} gtest gtest_main)
include_directories(${CMAKE_CURRENT_LIST_DIR})

project(${PROJECT_NAME}-test)

find_program(VALGRIND_EXE valgrind)
if (VALGRIND_EXE)
    set(CTEST_MEMORYCHECK_COMMAND "${VALGRIND_EXE}")
endif()

add_subdirectory(traits)
add_subdirectory(url)
# add_subdirectory(router)
add_subdirectory(http)
