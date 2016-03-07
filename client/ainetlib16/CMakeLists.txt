cmake_minimum_required(VERSION 3.1)

project(ainetlib16)
set(CMAKE_BUILD_TYPE Release)

# Dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
# SFML. Required version: 2+. Modules: network, system
find_package(SFML 2 COMPONENTS network system)

# Warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Library compilation
include_directories(.)
add_library(ainl16 SHARED ainetlib16.cpp)
target_link_libraries(ainl16 ${SFML_LIBRARIES})
target_compile_features(ainl16 PRIVATE cxx_range_for)

# Installation
install(TARGETS ainl16 DESTINATION lib)
install(FILES ainetlib16.hpp DESTINATION include)


# Wrap
SET(CMAKE_CXX_FLAGS "-std=c++11")
FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

SET(CMAKE_SWIG_FLAGS "")

SET_SOURCE_FILES_PROPERTIES(pyinterface.i PROPERTIES CPLUSPLUS ON)
SET_SOURCE_FILES_PROPERTIES(pyinterface.i PROPERTIES SWIG_FLAGS "-includeall")

# Python wrap
FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
SWIG_ADD_MODULE(pyainl16 python pyinterface.i ainetlib16.cpp)
SWIG_LINK_LIBRARIES(pyainl16 ${PYTHON_LIBRARIES} ${SFML_LIBRARIES})

# Java wrap
# find_package(Java REQUIRED)
# #find_package(JNI REQUIRED)
# include(UseJava)

# set(CMAKE_SWIG_FLAGS -package org.mycompany)
# set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR}/src/main/java/org/mycompany)

# swig_add_module(jainetlib16 java interface.i)
# include_directories(${JNI_INCLUDE_DIRS})

#swig_link_libraries(jainetlib16 Example)
