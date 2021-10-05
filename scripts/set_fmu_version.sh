#!/bin/bash

ver=$(git describe --tags --long)
xml=$1

sed -i '/\s*" numberOfEventIndicators/i \    version='$ver'"' $xml


