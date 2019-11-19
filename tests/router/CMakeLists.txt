project(${PROJECT_NAME}-router)

add_executable(${PROJECT_NAME}-basic basic.cpp)
add_test(NAME ${PROJECT_NAME}-basic COMMAND ${PROJECT_NAME}-basic)

add_executable(${PROJECT_NAME}-custom-type custom_type.cpp)
add_test(NAME ${PROJECT_NAME}-custom-type COMMAND ${PROJECT_NAME}-custom-type)

add_executable(${PROJECT_NAME}-return-value return_value.cpp)
add_test(NAME ${PROJECT_NAME}-return-value COMMAND ${PROJECT_NAME}-return-value)
