cmake_minimum_required(VERSION 2.8)

project(worm)

find_package(OpenMP REQUIRED)
find_package(Boost COMPONENTS program_options regex REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -std=c++0x ${OpenMP_CXX_FLAGS}")

set(sampler_SRCS aligned_tree.cc dictionary.cc distributed_rule_counts.cc
    node.cc pcfg_table.cc rule_reorderer.cc sampler.cc sampler_main.cc
    time_util.cc translation_table.cc util.cc)
add_executable(sampler ${sampler_SRCS})
target_link_libraries(sampler ${Boost_LIBRARIES})

set(reorder_SRCS aligned_tree.cc dictionary.cc grammar.cc
    multi_sample_reorderer.cc node.cc reorder_main.cc reorderer.cc
    rule_matcher.cc rule_reorderer.cc rule_stats_reporter.cc
    single_sample_reorderer.cc util.cc viterbi_reorderer.cc)
add_executable(reorder ${reorder_SRCS})
target_link_libraries(reorder ${Boost_LIBRARIES})

set(heuristic_SRCS aligned_tree.cc alignment_heuristic.cc dictionary.cc
    heuristic.cc node.cc time_util.cc translation_table.cc util.cc)
add_executable(heuristic ${heuristic_SRCS})
target_link_libraries(heuristic ${Boost_LIBRARIES})
