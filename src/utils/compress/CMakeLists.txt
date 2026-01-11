cmake_minimum_required(VERSION 3.14.0)

include_directories(${PROJECT_SOURCE_DIR}/src)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

include_directories(
    ${ZLIB_INCLUDE_DIRS}
    ${Zstd_INCLUDE_DIRS}
    ${Snappy_INCLUDE_DIR}
    ${LZ4_INCLUDE_DIR}
)
link_directories(
    ${ZLIB_LIBRARY_DIRS}
    ${Zstd_LIBRARY_DIRS}
    ${Snappy_LIBRARY_DIRS}
    ${LZ4_LIBRARY_DIRS}
)

find_library(ZSTD_STATIC_LIB NAMES libzstd.a zstd)
find_library(LZ4_STATIC_LIB NAMES liblz4.a lz4)
find_library(SNAPPY_STATIC_LIB NAMES libsnappy.a snappy)
find_library(ZLIB_STATIC_LIB NAMES libz.a zlib)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(BUILD_SHARED_LIBS OFF)

project(coev_compress)

aux_source_directory(. SRC_DIR)

add_library(${PROJECT_NAME} STATIC ${SRC_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${ZSTD_STATIC_LIB}
    ${LZ4_STATIC_LIB}
    ${SNAPPY_STATIC_LIB}
    ${ZLIB_STATIC_LIB}
)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})


if(BUILD_TESTS)
    # 添加测试程序
    add_executable(${PROJECT_NAME}_test test_compress.cpp)
    target_link_libraries(${PROJECT_NAME}_test GTest::GTest GTest::Main coev_compress)

    enable_testing()
    # 注册测试
    add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

endif()
