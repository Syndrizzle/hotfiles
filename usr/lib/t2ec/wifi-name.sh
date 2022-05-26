#!/bin/sh

# This script displays battery icon according to the charge level and charging state

# Author: Piotr Miller
# e-mail: nwg.piotr@gmail.com
# Website: http://nwg.pl
# Project: https://github.com/nwg-piotr/t2ec
# Icon by @edskeye

# Dependencies: `wireless_tools`
# Arguments: [-N] ("Wi-Fi: " instead of icon) | [-M'custom name']

name="none"

wifi=$(iwgetid | awk -F '"' '{ print $2 }')
if [[ ! -z "$wifi" ]]; then
    name="$wifi"
fi

if [[ $1 == -N* ]]; then
    echo "Wi-Fi: $name"
elif [[ $1 == -M* ]]; then
    echo "$(echo $1 | cut -c3-) $name"
else
    echo /usr/share/t2ec/network.svg
    echo "$name"
fi

