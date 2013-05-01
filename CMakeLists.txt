cmake_minimum_required(VERSION 2.8)

project(worm)

# Enable compiler optimization -O3
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

find_package(Boost COMPONENTS program_options regex REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

set(worm_SRCS aligned_tree.cc dictionary.cc main.cc node.cc util.cc)
add_executable(worm ${worm_SRCS})
target_link_libraries(worm ${Boost_LIBRARIES})
