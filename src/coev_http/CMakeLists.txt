cmake_minimum_required(VERSION 3.14.0)

include_directories(${PROJECT_SOURCE_DIR}/src)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

project(coev_http)

aux_source_directory(. SRC_DIR)

add_library(${PROJECT_NAME} STATIC ${SRC_DIR})
target_link_libraries(${PROJECT_NAME})