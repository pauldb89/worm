cmake_minimum_required(VERSION 2.8)

project(worm)

find_package(OpenMP REQUIRED)
find_package(Boost COMPONENTS filesystem program_options regex system REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -std=c++0x ${OpenMP_CXX_FLAGS}")

set(base_SRCS aligned_tree.cc dictionary.cc lattice.cc node.cc
    translation_table.cc util.cc)

set(sampler_SRCS alignment_constructor.cc distributed_rule_counts.cc
    pcfg_table.cc rule_extractor.cc rule_reorderer.cc sampler.cc sampler_main.cc
    time_util.cc ${base_SRCS})
add_executable(sampler ${sampler_SRCS})
target_link_libraries(sampler ${Boost_LIBRARIES})

set(reorder_SRCS grammar.cc multi_sample_reorderer.cc reorder_main.cc
    reorderer.cc rule_matcher.cc rule_reorderer.cc rule_stats_reporter.cc
    single_sample_reorderer.cc time_util.cc viterbi_reorderer.cc ${base_SRCS})
add_executable(reorder ${reorder_SRCS})
target_link_libraries(reorder ${Boost_LIBRARIES})

set(heuristic_SRCS alignment_heuristic.cc heuristic.cc time_util.cc ${base_SRCS})
add_executable(heuristic ${heuristic_SRCS})
target_link_libraries(heuristic ${Boost_LIBRARIES})

set(filter_SRCS alignment_constructor.cc distributed_rule_counts.cc filter.cc
    rule_extractor.cc ${base_SRCS})
add_executable(filter ${filter_SRCS})
target_link_libraries(filter ${Boost_LIBRARIES})

set(generate_alignments_SRCS alignment_constructor.cc generate_alignments.cc
    rule_extractor.cc ${base_SRCS})
add_executable(generate_alignments ${generate_alignments_SRCS})
target_link_libraries(generate_alignments ${Boost_LIBRARIES})
