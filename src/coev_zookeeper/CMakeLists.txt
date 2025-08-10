
cmake_minimum_required(VERSION 3.5)

include_directories(${PROJECT_SOURCE_DIR}/src)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

project(coev_zookeeper VERSION 3.10.0)


add_compile_options(-Wall -fPIC)

add_definitions(-DUSE_STATIC_LIB)

option(WANT_SYNCAPI "Enables Sync API support" ON)
if(WANT_SYNCAPI)
  add_definitions(-DTHREADED)
endif()

set(DEFAULT_WANT_CPPUNIT OFF)

option(WANT_CPPUNIT "Enables CppUnit and tests" ${DEFAULT_WANT_CPPUNIT})

option(WANT_SOCK_CLOEXEC "Enables SOCK_CLOEXEC on sockets" OFF)
include(CheckSymbolExists)
check_symbol_exists(SOCK_CLOEXEC sys/socket.h HAVE_SOCK_CLOEXEC)
if(WANT_SOCK_CLOEXEC AND HAVE_SOCK_CLOEXEC)
  set(SOCK_CLOEXEC_ENABLED 1)
endif()


option(WITH_CYRUS_SASL "turn ON/OFF Cyrus SASL 2.x support, or define SASL library location (default: ON)" ON)
message("-- using WITH_CYRUS_SASL=${WITH_CYRUS_SASL}")
if(NOT WITH_CYRUS_SASL STREQUAL "OFF")
  if(NOT WITH_CYRUS_SASL STREQUAL "ON")
    set(CYRUS_SASL_ROOT_DIR "${WITH_CYRUS_SASL}")
  endif()
  find_package(CyrusSASL)
  if(CYRUS_SASL_FOUND)
    message("-- Cyrus SASL 2.x found! will build with SASL support.")
  else()
    message("-- WARNING: unable to find Cyrus SASL 2.x! will build without SASL support.")
  endif()
endif()

set(zookeeper_sources
  proto.cpp
  ZooCli.cpp
  Zoo.cpp
  ZooOp.cpp
)

add_library(${PROJECT_NAME} STATIC ${zookeeper_sources})
target_include_directories(${PROJECT_NAME} PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(${PROJECT_NAME} PUBLIC rt)

option(WITH_OPENSSL "turn ON/OFF SSL support, or define openssl library location (default: ON)" ON)
message("-- using WITH_OPENSSL=${WITH_OPENSSL}")
if(NOT WITH_OPENSSL STREQUAL "OFF")
  if(NOT WITH_OPENSSL STREQUAL "ON")
    set(OPENSSL_ROOT_DIR,${WITH_OPENSSL})
  endif()
  find_package(OpenSSL)
  if(OPENSSL_FOUND)
    target_compile_definitions(${PROJECT_NAME} PUBLIC HAVE_OPENSSL_H)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenSSL::SSL OpenSSL::Crypto)
  else()
  endif()
endif()

if(WANT_SYNCAPI AND NOT WIN32)
  find_package(Threads REQUIRED)
  target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
endif()

if(CYRUS_SASL_FOUND)
  target_compile_definitions(${PROJECT_NAME} PUBLIC HAVE_CYRUS_SASL_H)
  target_link_libraries(${PROJECT_NAME} PUBLIC CyrusSASL)
endif()

