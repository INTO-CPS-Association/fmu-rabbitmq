#!/bin/bash
set -e

target=darwin-x64
install_name=darwin-x86_64

repo=$(git rev-parse --show-toplevel)
$repo/scripts/_build_base.sh $target $install_name
