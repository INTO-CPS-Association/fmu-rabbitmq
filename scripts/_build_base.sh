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
	if [ ! -d $2 ]
then

	 if [ $target == "linux-x64" ]; then
	   transcoder_option="-Dtranscoder=gnuiconv"
	else
	   transcoder_option=""
	fi
	#echo cmake $3 "$4" -B$1 -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON -DCMAKE_INSTALL_PREFIX=$2 -Dthreads:BOOL=OFF -Dnetwork:BOOL=OFF $transcoder_option -Hthirdparty/xerces-c

	if [[ "$target" == "win-x64" ]]
	then
		cmake -G "MSYS Makefiles" $3 "$4" -B$1 -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON -DCMAKE_INSTALL_PREFIX=$2 -Dthreads:BOOL=OFF -Dnetwork:BOOL=OFF $transcoder_option -Hthirdparty/xerces-c
	else
		cmake $3 "$4" -B$1 -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_CXX_EXTENSIONS=ON -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON -DCMAKE_INSTALL_PREFIX=$2 -Dthreads:BOOL=OFF -Dnetwork:BOOL=OFF $transcoder_option -Hthirdparty/xerces-c
	fi
	make -C$1 -j8
	make -C$1 install
fi
}

build_openssl()
{

if [ ! -d $2 ]
then

	c_dir=$(pwd)
	cd $1
	./Configure --prefix=$2 --openssldir=$2   '-Wl,-rpath,$(LIBRPATH)' no-docs no-tests
	make -j 9
	make install
	cd $c_dir
fi
}

 
mkdir -p build/external/$install_name
build_xercersc $working_dir/xerces-c build/external/$install_name/xerces-c
build_openssl $repo/thirdparty/openssl $(readlink -f "build/external/$install_name")/openssl
#build_xercersc $working_dir/xerces-c build/external/$install_name/xerces-c

echo Running CMake
rm -f thirdparty/rabbitmq-c/rabbitmq-c/librabbitmq/config.h
if [[ "$target" == "win-x64" ]]
then
	cmake -G "MSYS Makefiles" -B$working_dir -H.
else
	cmake -B$working_dir -H.
fi

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
