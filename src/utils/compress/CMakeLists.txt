cmake_minimum_required(VERSION 3.14.0)

include_directories(${PROJECT_SOURCE_DIR}/src)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(lz4_DIR "/usr/lib/x86_64-linux-gnu")
set(LZ4_INCLUDE_DIRS "/usr/include/lz4")
set(LZ4_LIBRARIES "/usr/lib/x86_64-linux-gnu/liblz4.a")
set(SNAPPY_INCLUDE_DIRS "/usr/include")
set(SNAPPY_LIBRARIES "/usr/lib/x86_64-linux-gnu/libsnappy.a")
set(ZSTD_INCLUDE_DIRS "/usr/include")
set(ZSTD_LIBRARIES "/usr/lib/x86_64-linux-gnu/libzstd.a")
set(ZLIB_INCLUDE_DIRS "/usr/include")
set(ZLIB_LIBRARIES "/usr/lib/x86_64-linux-gnu/libz.a")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

project(compress)

set(CMAKE_C_COMPILER gcc CACHE INTERNAL "cmake_c_compiler")
set(CMAKE_CXX_COMPILER g++ CACHE INTERNAL "cmake_cxx_compiler")
set(CMAKE_VERBOSE_MAKEFILE ON CACHE INTERNAL "cmake_verbose_makefile")
set(CMAKE_CXX_FLAGS "-O0 -ggdb -std=c++23 -fcoroutines" CACHE INTERNAL "cmake_cxx_flags")

include_directories(${LZ4_INCLUDE_DIRS})
include_directories(${SNAPPY_INCLUDE_DIRS})
include_directories(${ZSTD_INCLUDE_DIRS})
include_directories(${ZLIB_INCLUDE_DIRS})
link_directories(${LZ4_LIBRARIES})
link_directories(${SNAPPY_LIBRARIES})
link_directories(${ZSTD_LIBRARIES})
link_directories(${ZLIB_LIBRARIES})
aux_source_directory(. SRC_DIR)

add_library(${PROJECT_NAME} STATIC ${SRC_DIR})
target_link_libraries(${PROJECT_NAME} ${LZ4_LIBRARIES} ${SNAPPY_LIBRARIES} ${ZSTD_LIBRARIES} ${ZLIB_LIBRARIES})
