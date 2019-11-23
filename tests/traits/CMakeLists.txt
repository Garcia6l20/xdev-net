add_executable(${PROJECT_NAME}-function_traits function_traits.cpp)
target_link_libraries(${PROJECT_NAME}-function_traits GTest::GTest GTest::Main)
add_test(NAME ${PROJECT_NAME}-function_traits COMMAND ${PROJECT_NAME}-function_traits)
