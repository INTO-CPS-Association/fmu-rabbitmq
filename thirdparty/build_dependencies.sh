#!/bin/bash
set -e
#rm -rf external

lib=$(readlink -f external)


#cd  xerces-c
#rm CMakeCache.txt
# https://xerces.apache.org/xerces-c/build-3.html

build_xercersc()
{
 $1 cmake -B$2 -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON -DCMAKE_INSTALL_PREFIX=$3 -Dthreads:BOOL=OFF -Dnetwork:BOOL=OFF -Hxerces-c
 $1 make -C$2 -j8
 $1 make -C$2 install
}


# darwin
#lib_install_dir=$lib/darwin-x86_64
#if [ ! -d $lib_install_dir ]
#then
#docker run --rm dockcross/darwin-x64:latest > ./darwin-x64-dockcross
#chmod +x ./darwin-x64-dockcross
##mkdir $lib/darwin-x64
##build_xercersc ./darwin-x64-dockcross build-darwin-x64 darwin-x64-install
##./darwin-x64-dockcross cmake -Bbuild-darwin-x64 -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_INSTALL_PREFIX=darwin-x64-install -Dthreads:BOOL=OFF -Dnetwork:BOOL=OFF -H.
##./darwin-x64-dockcross make -Cbuild-darwin-x64 -j8
##./darwin-x64-dockcross make -Cbuild-darwin-x64 install
##mkdir -p $lib/darwin-x64
##mv darwin-x64-install/* $lib/darwin-x86_64
#
#build_dir=build-darwin-x64
#install_dir=${build_dir}-install
#
#build_xercersc ./darwin-x64-dockcross $build_dir $install_dir
#
#mkdir -p $lib_install_dir
#mv $install_dir/* $lib_install_dir
#
#fi

# linux
lib_install_dir=$lib/linux-x86_64
if [ ! -d $lib_install_dir ]
then
docker run --rm dockcross/linux-x64:latest > ./linux-x64-dockcross
chmod +x ./linux-x64-dockcross

build_dir=build-linux-x64
install_dir=${build_dir}-install

build_xercersc ./linux-x64-dockcross $build_dir $install_dir

mkdir -p $lib_install_dir
mv $install_dir/* $lib_install_dir
fi


# win
lib_install_dir=$lib/win-x86_64
if [ ! -d $lib_install_dir ]
then
docker run --rm dockcross/windows-static-x64:latest > ./win-x64-dockcross
chmod +x ./win-x64-dockcross

build_dir=build-win-x64
install_dir=${build_dir}-install

build_xercersc ./win-x64-dockcross $build_dir $install_dir

mkdir -p $lib_install_dir
mv $install_dir/* $lib_install_dir
fi

echo Done



