#!/bin/bash
set -e
#rm -rf external

lib=$(readlink -f external)
#cd  xerces-c
#rm CMakeCache.txt
# https://xerces.apache.org/xerces-c/build-3.html


# native mac
cmake -Bnative-xercersc -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_INSTALL_PREFIX=$lib/darwin-x86_64 -Dthreads:BOOL=OFF -Dnetwork:BOOL=OFF -Hxerces-c
make -Cnative-xercersc -j8
make -Cnative-xercersc install




