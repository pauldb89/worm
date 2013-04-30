cmake_minimum_required(VERSION 2.8)

project(worm)

# Enable compiler optimization -O3
set(CMAKE_BUILD_TYPE Release)

find_package(Boost COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_executable(worm main.cc)
target_link_libraries(worm ${Boost_LIBRARIES})
