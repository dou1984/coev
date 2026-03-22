cmake_minimum_required(VERSION 3.14.0)

include_directories(${PROJECT_SOURCE_DIR}/src)
link_directories(${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Main Kafka example
project(co_kafka)
aux_source_directory(./ SRC_DIR)
add_executable(${PROJECT_NAME} ${SRC_DIR})
target_link_libraries(${PROJECT_NAME} coev_kafka coev_dns coev_compress coev_hash coev_ssl cares ssl coev  crypto ev pthread)

# Admin test
project(admin_test)
add_executable(${PROJECT_NAME} admin_test.cpp)
target_link_libraries(${PROJECT_NAME} coev_kafka coev_dns coev_compress coev_hash coev_ssl cares ssl coev  crypto ev pthread)

# Producer test
project(producer_test)
add_executable(${PROJECT_NAME} producer_test.cpp)
target_link_libraries(${PROJECT_NAME} coev_kafka coev_dns coev_compress coev_hash coev_ssl cares ssl coev  crypto ev pthread)

# Consumer test
project(consumer_test)
add_executable(${PROJECT_NAME} consumer_test.cpp)
target_link_libraries(${PROJECT_NAME} coev_kafka coev_dns coev_compress coev_hash coev_ssl cares ssl coev  crypto ev pthread)
