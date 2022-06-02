#!/usr/bin/env bash
STATUS="$(bluetoothctl show | grep Powered | awk '{print $2}')"
toggle() {
    if [ $STATUS == "yes" ]; then
        bluetoothctl power off
        notify-send --icon=bluetooth-offline --urgency=normal "Bluetooth is now off." "Bluetooth has been turned off."
    else
        bluetoothctl power on
        notify-send --icon=volume-level-high --urgency=normal "Bluetooth is now on." "Bluetooth has been turned on."
    fi
}

icon() {
    # off
    if [ $(bluetoothctl show | grep "Powered: yes" | wc -c) -eq 0 ]
    then
        echo "󰂲"
    else
        # on
        if [ $(echo info | bluetoothctl | grep 'Device' | wc -c) -eq 0 ]
        then
            echo "󰂯"
            # connected
        else
            echo "󰂱"
        fi
    fi
}

status() {
    # off
    if [ $(bluetoothctl show | grep "Powered: yes" | wc -c) -eq 0 ]
    then
        echo "󰂲"
    else
        # on
        if [ $(echo info | bluetoothctl | grep 'Device' | wc -c) -eq 0 ]
        then
            echo "󰂯"
            # connected
        else
            echo "󰂯"
        fi
    fi
}

if [[ $1 == "--toggle" ]]; then
    toggle
    elif [[ $1 == "--status" ]]; then
    status
    elif [[ $1 == "--icon" ]]; then
    icon
fi