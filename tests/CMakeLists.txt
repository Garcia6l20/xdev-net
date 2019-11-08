include(FetchContent)

FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.10.0
)
FetchContent_MakeAvailable(googletest)

link_libraries(${PROJECT_NAME} gtest gtest_main gmock)

project(${PROJECT_NAME}-test)

enable_testing()

add_subdirectory(socket)
