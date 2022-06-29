#!/usr/bin/env bash

STATUS=$(dunstctl is-paused)

toggle() {
    if [ $STATUS == "false" ]; then
        dunstctl set-paused true
    else
        dunstctl set-paused false
        notify-send --urgency=normal "Do Not Disturb" "Do not Disturb has been turned off!"
    fi
}

icon() {
    if [ $STATUS == "false" ]; then
        echo "󱑙"
    else
        echo "󰍶"
    fi
}

status() {
    if [ $STATUS == "false" ]; then
        echo "Off"
    else
        echo "On"
    fi
}

if [[ $1 == "--toggle" ]]; then
    toggle
elif [[ $1 == "--icon" ]]; then
    icon
elif [[ $1 == "--status" ]]; then
    status
fi
