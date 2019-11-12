add_executable(${PROJECT_NAME}-http-basic http-basic.cpp ../net-test.hpp)
add_test(NAME ${PROJECT_NAME}-http-basic COMMAND ${PROJECT_NAME}-http-basic)
