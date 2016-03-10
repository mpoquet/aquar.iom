#!/bin/bash -e

# Remove previous export
rm -rf ./aquariom

##########
# Server #
##########
mkdir -p ./aquariom/bin
cp ../server/server ./aquariom/bin/

########
# Visu #
########
mkdir -p ./aquariom/bin
cp ../visu/visu ./aquariom/bin/

########
# Maps #
########
mkdir -p ./aquariom/maps
cp ../maps/map2p.json ./aquariom/maps/

######################
# C++ client example #
######################
mkdir -p ./aquariom/example_bot_cpp
cp ../client/ainetlib16/build/libainl16.so ./aquariom/example_bot_cpp/
cp ../bots/cpp/example/example.cpp ./aquariom/example_bot_cpp/

cat > ./aquariom/example_bot_cpp/Makefile.tmp << EOF
example: example.cpp
    g++ -std=c++11 -g -lainl16 -lsfml-network -lsfml-system example.cpp -o example
EOF
unexpand -t 4 --first-only ./aquariom/example_bot_cpp/Makefile.tmp > ./aquariom/example_bot_cpp/Makefile
rm ./aquariom/example_bot_cpp/Makefile.tmp

#########################
# Python client example #
#########################
mkdir -p ./aquariom/example_bot_python
cp ../client/ainetlib16/build/pyainl16.py ./aquariom/example_bot_python/
cp ../client/ainetlib16/build/_pyainl16.so ./aquariom/example_bot_python/
cp ../bots/python/example.py ./aquariom/example_bot_python/

#######################
# Java client example #
#######################
mkdir -p ./aquariom/example_bot_java/bots
mkdir -p ./aquariom/example_bot_java/bin/bots
cp ../client/ainetlib16/build/libjainl16.so ./aquariom/example_bot_java/
cp -r ../client/ainetlib16/build/src/main/java/org ./aquariom/example_bot_java/
cp ../bots/java/example/bots/Example.java ./aquariom/example_bot_java/bots/

cat > ./aquariom/example_bot_java/Makefile.tmp << EOF
bin/bots/Example.class: bots/Example.java
    javac -d bin -sourcepath . bots/Example.java

run: bin/bots/Example.class
    java -Djava.library.path=. -cp bin bots.Example ::1 4242
EOF
unexpand -t 4 --first-only ./aquariom/example_bot_java/Makefile.tmp > ./aquariom/example_bot_java/Makefile
rm ./aquariom/example_bot_java/Makefile.tmp

#########
# Proto #
#########
cp -r ../proto ./aquariom/

#####################
# API documentation #
#####################
mkdir -p ./aquariom/doc
cp -r ../client/ainetlib16/doc/html ./aquariom/doc/
cp ../client/ainetlib16/doc/latex/refman.pdf ./aquariom/doc
