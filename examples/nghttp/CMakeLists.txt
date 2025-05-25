cmake_minimum_required(VERSION 3.14.0)

include_directories(${PROJECT_SOURCE_DIR}/src)
link_directories(${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
project(test_nghttp)

aux_source_directory(./ SRC_DIR)
link_directories(/usr/local/lib64/ /usr/local/lib/)

add_executable(${PROJECT_NAME} ${SRC_DIR})
target_link_libraries(${PROJECT_NAME} coev_nghttp coev_ssl  nghttp2 coev ev pthread)
