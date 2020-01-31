#!/bin/bash

repo=$(git rev-parse --show-toplevel)
current_dir=$(pwd)
cd $repo

cd build/install/rabbitmqfmu
zip -r ../../rabbitmq.fmu . -i '*.dylib' '*.so' '*.xml' '*.dll'

cd $current_dir

echo "FMU packed build/rabbitmq.fmu"
