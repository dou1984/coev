cmake_minimum_required(VERSION 3.14.0)

project(coev_kafka)

include_directories(${PROJECT_SOURCE_DIR}/src)
set(CMAKE_C_COMPILER gcc-12)
set(CMAKE_CXX_COMPILER g++-12)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

aux_source_directory(. SRC_DIR)

set(CMAKE_CXX_FLAGS " -std=c++20 -fcoroutines -O0" CACHE INTERNAL "cmake_cxx_flags")

include_directories(../coev)
add_library(${PROJECT_NAME} STATIC ${SRC_DIR})
target_link_libraries(${PROJECT_NAME})
