#!/bin/bash -e

cd ..
base_dir=$(pwd)

# Server
cd ${base_dir}
cd server
qmake
make clean
make -j 4

# Client API
cd ${base_dir}
cd client/ainetlib16
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make clean
make -j 4
sudo make install

# Visu
cd ${base_dir}
cd visu
qmake
make clean
make -j 4

# C++ bots
cd ${base_dir}
cd bots/cpp
qmake
make clean
make -j 4

# Client C++ API documentation
cd ${base_dir}
cd client/ainetlib16
doxygen
cd doc/latex
make
