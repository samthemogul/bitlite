cmake_minimum_required(VERSION 3.10)
project(BittorrentClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include path
include_directories(include)

# Collect all source files from src/
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Add executable
add_executable(bitlite ${SOURCES})

# Find OpenSSL
find_package(OpenSSL REQUIRED)
target_link_libraries(bitlite PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# Find Boost
find_package(Boost REQUIRED COMPONENTS system thread)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(bitlite PRIVATE Boost::boost Boost::system Boost::thread)
endif()

# Add include path for libnl
include_directories(/usr/include/libnl3)

# Link Netlink libraries
find_library(NL3_LIB nl-3 REQUIRED)
find_library(NL_GENL3_LIB nl-genl-3 REQUIRED)
target_link_libraries(bitlite PRIVATE ${NL3_LIB} ${NL_GENL3_LIB})

# Link pthread for multithreading
find_package(Threads REQUIRED)
target_link_libraries(bitlite PRIVATE Threads::Threads)