add_executable(${PROJECT_NAME}-http-basic http-basic.cpp ../net-test.hpp)
add_test(NAME ${PROJECT_NAME}-http-basic COMMAND ${PROJECT_NAME}-http-basic)

add_executable(${PROJECT_NAME}-https-basic https-basic.cpp ../net-test.hpp)
add_test(NAME ${PROJECT_NAME}-https-basic COMMAND ${PROJECT_NAME}-https-basic)
