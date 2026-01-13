cmake_minimum_required(VERSION 3.14.0)

include_directories(${PROJECT_SOURCE_DIR}/src)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

project(coev_kafka)

# Get all source files excluding test files
file(GLOB SRC_FILES "*.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX ".*_test\.cpp$")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_link_libraries(${PROJECT_NAME})

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

if(BUILD_TESTS)
    # 添加测试程序
    add_executable(${PROJECT_NAME}_test 
        metadata_request_test.cpp 
        metadata_response_test.cpp 
        control_record_test.cpp
        message_test.cpp
        acl_types_test.cpp
        admin_test.cpp
        add_offsets_to_txn_request_test.cpp
        async_producer_test.cpp
        balance_strategy_test.cpp
        main_test.cpp
    )
    target_link_libraries(${PROJECT_NAME}_test 
        GTest::GTest 
        GTest::Main 
        coev_kafka 
        coev_dns 
        coev_compress 
        coev_hash 
        coev_ssl 
        cares 
        ssl 
        coev 
        crypto 
        ev 
        pthread
    )

    enable_testing()
    # 注册测试
    add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME}_test)

endif()
