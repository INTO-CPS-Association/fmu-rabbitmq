#!/bin/bash
set -e

target=win-x86
install_name=win-i386
dockcross_url=dockcross/windows-static-x86:latest

$(git rev-parse --show-toplevel)/scripts/build.sh $target $install_name $dockcross_url
