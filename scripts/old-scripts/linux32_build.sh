#!/bin/bash
set -e

target=linux-x86
install_name=linux-i386
dockcross_url=dockcross/${target}:latest

$(git rev-parse --show-toplevel)/scripts/build.sh $target $install_name $dockcross_url
