cmake_minimum_required(VERSION 3.14.0)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

project(coloop)

aux_source_directory(. SRC_DIR)

add_library(${PROJECT_NAME} STATIC ${SRC_DIR})
target_link_libraries(${PROJECT_NAME})