cmake_minimum_required(VERSION 3.8)
project(tcp_ip)
set(LIB_NAME tcp_ip)

include_directories(./include)

# Select flags.
set(CMAKE_CXX_FLAGS "-O0 -Wall -fsanitize=address")

set(CMAKE_CXX_STANDARD 17)

file(GLOB proj_HEADERS "include/*.h")
file(GLOB proj_SOURCES "src/*.cpp")

# Add core library.
include(FindPCAP.cmake)

add_library(${LIB_NAME} SHARED ${proj_HEADERS} ${proj_SOURCES})
target_link_libraries(tcp_ip ${PCAP_LIBRARY})

# Add subdirectories.
# add_subdirectory(examples)
# add_subdirectory(tests)
