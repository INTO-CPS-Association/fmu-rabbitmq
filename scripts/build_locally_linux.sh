#!/bin/bash
set -e

target=linux-x64
install_name=linux-x86_64

repo=$(git rev-parse --show-toplevel)
$repo/scripts/_build_base.sh $target $install_name
