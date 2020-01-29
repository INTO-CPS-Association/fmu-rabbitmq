#!/bin/bash

repo=$(git rev-parse --show-toplevel)
current_dir=$(pwd)
cd $repo

cd build/install/rabbitmq-fmu
zip -r ../../rabbitmq.fmu .

cd $current_dir

echo "FMU packed build/rabbitmq.fmu"
