cmake_minimum_required(VERSION 2.8)

project(worm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -std=c++0x")

find_package(Boost COMPONENTS program_options regex REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

set(worm_SRCS aligned_tree.cc dictionary.cc main.cc node.cc pcfg_table.cc sampler.cc translation_table.cc util.cc)
add_executable(worm ${worm_SRCS})
target_link_libraries(worm ${Boost_LIBRARIES})
