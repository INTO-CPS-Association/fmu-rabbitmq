#!/bin/bash
set -e

echo Creating env

docker run --rm dockcross/linux-x64:latest >./linux-x64-dockcross
chmod +x ./linux-x64-dockcross

echo Running CMake

./linux-x64-dockcross cmake -Bbuild/linux-x64 -H.

echo Compiling

./linux-x64-dockcross make -Cbuild/linux-x64
