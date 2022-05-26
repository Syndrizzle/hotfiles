#!/usr/bin/env bash
STATUS=$(awk '/%/ {gsub(/[\[\]]/,""); print $6}' <(amixer sget Master) | awk 'FNR == 1')
toggle() {
    if [ $STATUS == "on" ]; then
        amixer set Master toggle
        notify-send --icon=volume-level-muted --urgency=normal "Volume is muted." "Volume has been muted, your PC has been shut the fucked up."
    else
        amixer set Master toggle
        notify-send --icon=volume-level-high --urgency=normal "Volume has been turned on." "Sound is now on, you will hear farts!"
    fi
}

status() {
    if [ $STATUS == "on" ]; then
        echo "󰕾"
    else
        echo "󰖁"
    fi
}

if [[ $1 == "--toggle" ]]; then
    toggle
    elif [[ $1 == "--status" ]]; then
    status
fi