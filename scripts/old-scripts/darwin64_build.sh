#!/bin/bash
set -e

target=darwin-x64
install_name=darwin-x86_64
dockcross_url=docker.sweng.au.dk/dockcross-darwin-x64-gcc:cmake3.19.0


$(git rev-parse --show-toplevel)/scripts/build.sh $target $install_name $dockcross_url
