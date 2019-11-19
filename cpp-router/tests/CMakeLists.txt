link_libraries(XDev::Router GTest::GTest GTest::Main)
include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(${PROJECT_NAME}-basic-test
    basic.cpp
)
add_executable(${PROJECT_NAME}-custrom-type-test
    custom_type.cpp
)
add_executable(${PROJECT_NAME}-return-value-test
    return_value.cpp
)
