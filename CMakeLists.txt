cmake_minimum_required(VERSION 3.14.0)

project(coev)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE INTERNAL "cmake_build_type")
  #SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

add_subdirectory(./src)
add_subdirectory(./examples)

set(CMAKE_C_COMPILER gcc CACHE INTERNAL "cmake_c_compiler")
set(CMAKE_CXX_COMPILER g++ CACHE INTERNAL "cmake_cxx_compiler")
set(CMAKE_VERBOSE_MAKEFILE ON CACHE INTERNAL "cmake_verbose_makefile")
set(CMAKE_CXX_FLAGS "-ggdb -std=c++2a -fcoroutines" CACHE INTERNAL "cmake_cxx_flags")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib CACHE INTERNAL "cmake_library_output_directory")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib CACHE INTERNAL "cmake_archive_output_directory")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin CACHE INTERNAL "cmake_runtime_output_directory") 

include_directories(/usr/local/include )
link_directories(/usr/local/lib64/ /usr/local/lib/)

# add_executable(${PROJECT_NAME} ${SRC_DIR})
# add_library(${PROJECT_NAME} STATIC ${SRC_DIR})
# target_link_libraries(${PROJECT_NAME})

# install(TARGETS ${PROJECT_NAME}
# LIBRARY DESTINATION lib
# PUBLIC_HEADER DESTINATION include/${PROJECT_NAME})
