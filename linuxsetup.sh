#!/bin/sh -e

make --version
qmake --version
cmake --version

CORES=3 #${nproc}

git submodule update --init --recursive

cd libs
mkdir -p build_limereport
cd build_limereport
qmake ../LimeReport/limereport.pro -spec linux-g++
make -j$CORES qmake_all
make -j$CORES
cd ../..

cd libs/googletest
mkdir -p build
cd build
cmake -G "Unix Makefiles" ..
make -j$CORES
echo Setup complete
