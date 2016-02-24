cmake_minimum_required(VERSION 3.1)

project(ainetlib16)
set(CMAKE_BUILD_TYPE Release)

# Dependencies
# SFML. Required version: 2+. Modules: network, system
find_package(SFML 2 COMPONENTS network system)

# Warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Library compilation
include_directories(.)
add_library(ainetlib16 SHARED ainetlib16.cpp)
target_compile_features(ainetlib16 PRIVATE cxx_range_for)

# Installation
install(TARGETS ainetlib16 DESTINATION lib)
install(FILES ainetlib16.hpp DESTINATION include)
