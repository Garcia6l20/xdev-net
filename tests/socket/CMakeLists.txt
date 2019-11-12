add_executable(${PROJECT_NAME}-udp udp.cpp ../net-test.hpp)
add_test(NAME ${PROJECT_NAME}-udp COMMAND ${PROJECT_NAME}-udp)

add_executable(${PROJECT_NAME}-tcp tcp.cpp ../net-test.hpp)
add_test(NAME ${PROJECT_NAME}-tcp COMMAND ${PROJECT_NAME}-tcp)
