cmake_minimum_required(VERSION 3.14)

enable_testing()

include_directories(${PROJECT_SOURCE_DIR}/src)
link_directories(${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/tests)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

project(common_test)
# add_subdirectory(googletest)  # 如果 googletest 是子模块
# add_executable(main serializer.cpp)

# 添加测试程序
add_executable(${PROJECT_NAME} test_serializer.cpp test_compress.cpp)
target_link_libraries(${PROJECT_NAME} GTest::GTest GTest::Main serializer compress)

# 注册测试
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
