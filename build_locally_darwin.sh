#!/bin/bash
set -e

target=darwin-x64
install_name=darwin-x86_64

echo "Configuration Target=${target}, DependencyInstallName=${install_name}"

repo=$(git rev-parse --show-toplevel)
current_dir=$(pwd)
cd $repo


working_dir=build/$target

mkdir -p $working_dir

echo Build dependencies


build_xercersc()
{
 cmake -B$1 -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON -DCMAKE_INSTALL_PREFIX=$2 -Dthreads:BOOL=OFF -Dnetwork:BOOL=OFF -Hthirdparty/xerces-c
 make -C$1 -j8
 make -C$1 install
}

if [ ! -d build/external/$install_name ]
then

build_xercersc $working_dir/xerces-c build/external/$install_name
else
echo "Dependency already generated"
fi


echo Running CMake
rm -f thirdparty/rabbitmq-c/rabbitmq-c/librabbitmq/config.h
cmake -B$working_dir -H.

echo Compiling

make -C$working_dir -j8
echo $MY_INSTALL_DIR
make -C$working_dir/rabbitmq-fmu install
cd $current_dir

echo "TESTING STARTED"
echo $working_dir
echo $current_dir

ctest --test-dir build/$target -R unit-test-rabbitmq 
echo Done
