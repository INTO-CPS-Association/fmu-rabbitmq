#!/bin/bash
set -e

target=$1
install_name=$2
dockcross_url=$3

echo "Configuration Target=${target}, DependencyInstallName=${install_name}, Dockcross image url=${dockcross_url}"

repo=$(git rev-parse --show-toplevel)
current_dir=$(pwd)
cd $repo


working_dir=build/$target

mkdir -p $working_dir

script=$working_dir/${target}-dockcross


echo Creating env

docker run --rm $dockcross_url > $script
chmod +x $script


echo Build dependencies


build_xercersc()
{
 $1 cmake -B$2 -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON -DCMAKE_INSTALL_PREFIX=$3 -Dthreads:BOOL=OFF -Dnetwork:BOOL=OFF -Hthirdparty/xerces-c
 $1 make -C$2 -j8
 $1 make -C$2 install
}

if [ ! -d thirdparty/external/$install_name ]
then

build_xercersc $script $working_dir/xerces-c thirdparty/external/$install_name
else
echo "Dependency already generated"
fi


echo Running CMake

./$script cmake -B$working_dir -H.

echo Compiling

./$script make -C$working_dir
./$script make -C$working_dir/rabbitmq-fmu install
cd $current_dir

echo Done
