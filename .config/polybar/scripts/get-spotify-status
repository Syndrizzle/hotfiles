#!/bin/bash

STATUS=$(playerctl -p spotify status 2> /dev/null)

if [[ $STATUS == "Playing" ]]; then
    echo "󰝚󰎇"
elif [[ $STATUS == "Paused" ]]; then
    echo "󰝚󰎇"
else
    echo ""
fi
