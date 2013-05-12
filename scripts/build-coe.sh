#!/bin/bash

DIR=$(dirname $0)
cd $DIR/..
pwd

export FSTRAIN_HOME=$(pwd)

cd build
../configure --with-boost=$HOME/software/boost-1.39 --with-openfst=$HOME/software/openfst-1.1 CXXFLAGS="-O2 -fPIC" 
export LD_LIBRARY_PATH=$(pwd):$HOME/software/openfst-1.1/lib:$HOME/software/boost-1.39/lib
make

cd ../test
make BIN_DIR=$(pwd)/../build

cd ../
cd build-debug
../configure --with-boost=$HOME/software/boost-1.39 --with-openfst=$HOME/software/openfst-1.1 --enable-debug CXXFLAGS="-fPIC" 
export LD_LIBRARY_PATH=$(pwd):$HOME/software/openfst-1.1/lib:$HOME/software/boost-1.39/lib
make

cd ../test
make BIN_DIR=$(pwd)/../build-debug

