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

#SET(CMAKE_SWIG_FLAGS "-modern")

# Python wrap
FIND_PACKAGE(PythonLibs 3 REQUIRED)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
SET_SOURCE_FILES_PROPERTIES(pyinterface.i PROPERTIES CPLUSPLUS ON)
SET_SOURCE_FILES_PROPERTIES(pyinterface.i PROPERTIES SWIG_FLAGS "-includeall")
SWIG_ADD_MODULE(pyainl16 python pyinterface.i ainetlib16.cpp)
SWIG_LINK_LIBRARIES(pyainl16 ${PYTHON_LIBRARIES} ${SFML_LIBRARIES})

# Java wrap
find_package(Java REQUIRED)
find_package(JNI)
include(UseJava)
SET_SOURCE_FILES_PROPERTIES(jinterface.i PROPERTIES CPLUSPLUS ON)
SET_SOURCE_FILES_PROPERTIES(jinterface.i PROPERTIES SWIG_FLAGS "-includeall")

set(CMAKE_SWIG_FLAGS -package org.contest16)
set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR}/src/main/java/org/contest16)

swig_add_module(jainl16 java jinterface.i ainetlib16.cpp)
include_directories(${JNI_INCLUDE_DIRS})

swig_link_libraries(jainl16 ${JAVA_LIBRARIES} ${SFML_LIBRARIES})

# Documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        ainetlib16.hpp ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif()
