#!/bin/bash

# Remove previous exports
rm -rf ./aquariom

# C++
mkdir -p ./aquariom/example_bot_cpp
cp ../client/ainetlib16/build/libainl16.so ./aquariom/example_bot_cpp/
cp ../bots/cpp/example/example.cpp ./aquariom/example_bot_cpp/

cat > ./aquariom/example_bot_cpp/Makefile.tmp << EOF
example: example.cpp
    g++ -std=c++11 -g -lainl16 -lsfml-network -lsfml-system example.cpp -o example
EOF
unexpand -t 4 --first-only ./aquariom/example_bot_cpp/Makefile.tmp > ./aquariom/example_bot_cpp/Makefile
rm ./aquariom/example_bot_cpp/Makefile.tmp

# Python
#mkdir -p ./aquariom/example_bot_python

# Java
#mkdir -p ./aquariom/example_bot_java
