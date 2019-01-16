#!/bin/bash -e

if [ ! -e build.ninja ] ; then
    cmake -DCMAKE_BUILD_TYPE=Debug -GNinja CMakeLists.txt .
fi

ninja $*
