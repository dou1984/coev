cmake_minimum_required(VERSION 3.14.0)

include_directories(/home/dou1984/github/coev/src/)

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
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -fcoroutines)
target_link_libraries(${PROJECT_NAME})

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

if(BUILD_TESTS)
    # Get all test files
    file(GLOB TEST_FILES "*_test.cpp")
    
    # Exclude duplicate test files
    list(FILTER TEST_FILES EXCLUDE REGEX ".*fetch_test\.cpp$")
    list(FILTER TEST_FILES EXCLUDE REGEX ".*offset_fetch_test\.cpp$")
    
    # 添加测试程序
    add_executable(${PROJECT_NAME}_test ${TEST_FILES})
    target_compile_features(${PROJECT_NAME}_test PRIVATE cxx_std_20)
    target_compile_options(${PROJECT_NAME}_test PRIVATE -fcoroutines)
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
