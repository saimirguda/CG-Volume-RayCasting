#!/bin/bash

mkdir -p build/bin && cd build/bin
cmake ../cmake
make -j
cd ../../
build/bin/bin/task2
