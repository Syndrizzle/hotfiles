#!/usr/bin/env bash
STATUS=$(nmcli | grep wlp4s0 | awk 'FNR == 1 {print $2}')
toggle() {
    if [ $STATUS == "connected" ]; then
        nmcli radio wifi off
        notify-send --icon=window-close --urgency=normal "Wi-Fi turned off." "Wi-Fi has been turned off, you are not connected to the outer world!"
    else
        nmcli radio wifi on
        notify-send --icon=checkmark --urgency=normal "Wi-Fi has been turned on." "Wi-Fi has been turned on, you are back online!"
    fi
}

status() {
    if [ $STATUS == "connected" ]; then
        echo "󰤨"
    else
        echo "󰤭"
    fi
}

if [[ $1 == "--toggle" ]]; then
    toggle
    elif [[ $1 == "--status" ]]; then
    status
fi