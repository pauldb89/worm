cmake_minimum_required(VERSION 2.8)

project(worm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -std=c++0x")

find_package(Boost COMPONENTS program_options regex REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

set(sampler_SRCS aligned_tree.cc dictionary.cc node.cc pcfg_table.cc sampler.cc sampler_main.cc translation_table.cc util.cc)
add_executable(sampler ${sampler_SRCS})
target_link_libraries(sampler ${Boost_LIBRARIES})

set(reorder_SRCS aligned_tree.cc dictionary.cc grammar.cc node.cc reorder_main.cc util.cc viterbi_reorderer.cc)
add_executable(reorder ${reorder_SRCS})
target_link_libraries(reorder ${Boost_LIBRARIES})
