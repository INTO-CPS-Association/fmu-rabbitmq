#!/bin/bash

ver=$(git describe --tags --long)
xml=$1

sed -i 's/generator/& '$ver'/' $xml


