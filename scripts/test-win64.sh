#!/bin/bash

repo=$(git rev-parse --show-toplevel)
current_dir=$(pwd)

cd $repo/build/win-x64
docker run -it -v $(pwd):/work scottyhardy/docker-wine /bin/bash -c 'cd /work/rabbitmq-fmu/ && wine ./unit-test-rabbitmq-fmu.exe'

cd $current_dir
