#!/bin/bash
set -e

target=win-x64
install_name=win-x86_64
dockcross_url=dockcross/windows-static-x64:latest

$(git rev-parse --show-toplevel)/scripts/build.sh $target $install_name $dockcross_url
