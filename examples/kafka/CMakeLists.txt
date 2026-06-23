cmake_minimum_required(VERSION 3.14.0)

include_directories(${PROJECT_SOURCE_DIR}/src)
link_directories(${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Main Kafka example
project(co_kafka)
add_executable(${PROJECT_NAME} main.cpp producer_test.cpp consumer_test.cpp supported_versions.cpp)
target_link_libraries(${PROJECT_NAME} coev_kafka coev_dns coev_compress coev_hash coev_ssl cares ssl coev  crypto ev pthread)

# Admin test
project(admin_test)
add_executable(${PROJECT_NAME} admin_test.cpp)
target_link_libraries(${PROJECT_NAME} coev_kafka coev_dns coev_compress coev_hash coev_ssl cares ssl coev  crypto ev pthread)

# Producer test
project(producer_test)
add_executable(${PROJECT_NAME} producer_test.cpp producer_test_main.cpp supported_versions.cpp)
target_link_libraries(${PROJECT_NAME} coev_kafka coev_dns coev_compress coev_hash coev_ssl cares ssl coev  crypto ev pthread)

# Consumer test
project(consumer_test)
add_executable(${PROJECT_NAME} consumer_test.cpp consumer_test_main.cpp)
target_link_libraries(${PROJECT_NAME} coev_kafka coev_dns coev_compress coev_hash coev_ssl cares ssl coev  crypto ev pthread)

# All messages version test
project(all_messages_version_test)
add_executable(${PROJECT_NAME} all_messages_version_test.cpp supported_versions.cpp)
target_link_libraries(${PROJECT_NAME} coev_kafka coev_dns coev_compress coev_hash coev_ssl cares ssl coev  crypto ev pthread)

# Admin version test
project(admin_version_test)
add_executable(${PROJECT_NAME} admin_version_test.cpp supported_versions.cpp)
target_link_libraries(${PROJECT_NAME} coev_kafka coev_dns coev_compress coev_hash coev_ssl cares ssl coev  crypto ev pthread)

# All messages test (encode/decode)
project(all_messages_test)
add_executable(${PROJECT_NAME} all_messages_test.cpp supported_versions.cpp)
target_link_libraries(${PROJECT_NAME} coev_kafka coev_dns coev_compress coev_hash coev_ssl cares ssl coev  crypto ev pthread)
