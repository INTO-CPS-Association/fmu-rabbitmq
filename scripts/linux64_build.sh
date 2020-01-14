#!/bin/bash
set -e

target=linux-x64
install_name=linux-x86_64
dockcross_url=dockcross/${target}:latest

$(git rev-parse --show-toplevel)/scripts/build.sh $target $install_name $dockcross_url
