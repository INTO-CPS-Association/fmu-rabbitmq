#!/bin/bash

ver=$(git describe --tags --long)
xml=$1

sed -i '/\s*<LogCategories>/i \    <RabbitMQVersion version="'$ver'"/>\n' $xml


