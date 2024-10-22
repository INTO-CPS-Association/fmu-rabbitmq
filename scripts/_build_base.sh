#!/bin/bash
set -e

target=$1 #linux-x64
install_name=$2 #linux-x86_64

echo "Configuration Target=${target}, DependencyInstallName=${install_name}"

current_dir=$(pwd)

repo=$(git rev-parse --show-toplevel)

cd $repo

working_dir=build/$target

mkdir -p $working_dir

echo Build dependencies


build_xercersc()
{
	 if [ $target == "linux-x64" ]; then
	   transcoder_option="-Dtranscoder=gnuiconv"
	else
	   transcoder_option=""
	fi
	echo cmake $3 "$4" -B$1 -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON -DCMAKE_INSTALL_PREFIX=$2 -Dthreads:BOOL=OFF -Dnetwork:BOOL=OFF $transcoder_option -Hthirdparty/xerces-c
	cmake $3 "$4" -B$1 -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON -DCMAKE_INSTALL_PREFIX=$2 -Dthreads:BOOL=OFF -Dnetwork:BOOL=OFF $transcoder_option -Hthirdparty/xerces-c
	make -C$1 -j8
	make -C$1 install
}

build_openssl()
{

	c_dir=$(pwd)
	cd $1
	./Configure --prefix=$2 --openssldir=/usr/local/ssl     '-Wl,-rpath,$(LIBRPATH)'
	make -j 9
	make install
	cd $c_dir
}

if [ ! -d build/external/$install_name ]
then

build_xercersc $working_dir/xerces-c build/external/$install_name
build_openssl $repo/thirdparty/openssl $(readlink -f "$working_dir/build/external/$install_name")
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

ctest --test-dir build/$target --output-on-failure -R unit-test-rabbitmq


echo Done
